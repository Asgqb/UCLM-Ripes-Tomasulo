#include "processortab.h"
#include "ui_processortab.h"

#include <QDebug>
#include <QDir>
#include <QFontMetrics>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QTemporaryFile>
#include <QVBoxLayout>

#include "cacheselectiondialog.h"
#include "cachesim/cachetypes.h"
#include "consolewidget.h"
#include "instructionmodel.h"
#include "pipelinediagrammodel.h"
#include "pipelinediagramwidget.h"
#include "processorhandler.h"
#include "processorregistry.h"
#include "processorselectiondialog.h"
#include "registercontainerwidget.h"
#include "registermodel.h"
#include "ripessettings.h"
#include "syscall/systemio.h"
#include "tomasulo/tomasulo.h"

#include "VSRTL/graphics/vsrtl_widget.h"

#include "processors/interface/ripesprocessor.h"

namespace Ripes {

static QString convertToSIUnits(const double l_value, int precision = 2) {
  QString unit;
  double value;

  if (l_value < 0) {
    value = l_value * -1;
  } else {
    value = l_value;
  }

  if (value >= 1000000 && value < 1000000000) {
    value = value / 1000000;
    unit = "M";
  } else if (value >= 1000 && value < 1000000) {
    value = value / 1000;
    unit = "K";
  } else if (value >= 1 && value < 1000) {
    value = value * 1;
  } else if ((value * 1000) >= 1 && value < 1000) {
    value = value * 1000;
    unit = "m";
  } else if ((value * 1000000) >= 1 && value < 1000000) {
    value = value * 1000000;
    unit = QChar(0x00B5);
  } else if ((value * 1000000000) >= 1 && value < 1000000000) {
    value = value * 1000000000;
    unit = "n";
  }

  if (l_value > 0) {
    return (QString::number(value, 10, precision) + " " + unit);
  } else if (l_value < 0) {
    return (QString::number(value * -1, 10, precision) + " " + unit);
  }
  return QString::number(0) + " ";
}

static QSpinBox *createTomasuloSpinBox(QWidget *parent, int minimum,
                                       int maximum,
                                       const QString &suffix = QString()) {
  auto *spinBox = new QSpinBox(parent);

  spinBox->setRange(minimum, maximum);
  spinBox->setAlignment(Qt::AlignRight);

  // Ancho suficiente para mostrar textos como "2 cycles", "10 cycles"
  // y "40 cycles" sin que Qt los recorte con "...".
  spinBox->setMinimumWidth(118);
  spinBox->setMaximumWidth(140);
  spinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  if (!suffix.isEmpty()) {
    spinBox->setSuffix(suffix);
  }

  return spinBox;
}

static QLabel *createTomasuloSectionLabel(const QString &text,
                                          QWidget *parent) {
  auto *label = new QLabel(text, parent);
  QFont font = label->font();
  font.setBold(true);
  label->setFont(font);
  return label;
}

ProcessorTab::ProcessorTab(QToolBar *controlToolbar,
                           QToolBar *additionalToolbar, QWidget *parent)
    : RipesTab(additionalToolbar, parent), m_ui(new Ui::ProcessorTab) {
  m_ui->setupUi(this);

  m_vsrtlWidget = m_ui->vsrtlWidget;

  setupTomasuloOptionsWidget();

  m_tomasuloWidget = new TomasuloWidget(m_ui->pipelinesplitter);
  m_tomasuloWidget->setMinimumHeight(430);

  syncTomasuloWithEditor();

  connect(RipesSettings::getObserver(RIPES_SETTING_SOURCECODE),
          &SettingObserver::modified, this, [this](const auto &) {
            if (m_tomasuloWidget) {
              syncTomasuloWithEditor();
            }
          });

  bool inserted = false;

  // Insertamos Tomasulo en el splitter, pero NO destruimos el VSRTLWidget.
  // Ripes sigue usando VSRTLWidget internamente para cargar procesadores,
  // sincronizar, resetear, etc.
  const int vsrtlIndex = m_ui->pipelinesplitter->indexOf(m_ui->vsrtlWidget);

  if (vsrtlIndex >= 0) {
    m_ui->pipelinesplitter->insertWidget(vsrtlIndex, m_tomasuloWidget);
    inserted = true;
  }

  if (!inserted) {
    qDebug() << "No se ha podido insertar TomasuloWidget en pipelinesplitter";
  }

  if (ProcessorHandler::isVSRTLProcessor()) {
    // Load the default constructed processor to the VSRTL widget. Do a bit of
    // sanity checking to ensure that the layout stored in the settings is valid
    // for the given processor
    unsigned layoutID =
        RipesSettings::value(RIPES_SETTING_PROCESSOR_LAYOUT_ID).toInt();
    const Layout *layout = nullptr;
    if (layoutID >= ProcessorRegistry::getDescription(ProcessorHandler::getID())
                        .layouts.size()) {
      layoutID = 0;
    }
    const auto &layouts =
        ProcessorRegistry::getDescription(ProcessorHandler::getID()).layouts;
    if (layouts.size() > layoutID) {
      layout = &layouts.at(layoutID);
    }
    loadProcessorToWidget(layout);

    // By default, lock the VSRTL widget
    m_vsrtlWidget->setLocked(true);
  }

  m_stageModel = new PipelineDiagramModel(this);

  updateInstructionModel();
  connect(ProcessorHandler::get(), &ProcessorHandler::procStateChangedNonRun,
          this, &ProcessorTab::updateStatistics);
  connect(ProcessorHandler::get(), &ProcessorHandler::procStateChangedNonRun,
          this, &ProcessorTab::updateInstructionLabels);
  connect(ProcessorHandler::get(), &ProcessorHandler::procStateChangedNonRun,
          this, [this] {
            m_reverseAction->setEnabled(m_vsrtlWidget->isReversible() &&
                                        !m_autoClockAction->isChecked());
          });

  setupSimulatorActions(controlToolbar);

  // Setup statistics update timer - this timer is distinct from the
  // ProcessorHandler's update timer, given that it needs to run during
  // 'running' the processor.
  m_statUpdateTimer = new QTimer(this);
  m_statUpdateTimer->setInterval(
      1000.0 / RipesSettings::value(RIPES_SETTING_UIUPDATEPS).toInt());
  connect(m_statUpdateTimer, &QTimer::timeout, this,
          &ProcessorTab::updateStatistics);
  connect(RipesSettings::getObserver(RIPES_SETTING_UIUPDATEPS),
          &SettingObserver::modified, m_statUpdateTimer, [this] {
            m_statUpdateTimer->setInterval(
                1000.0 /
                RipesSettings::value(RIPES_SETTING_UIUPDATEPS).toInt());
          });

  // Connect changes in VSRTL reversible stack size to checking whether the
  // simulator is reversible
  connect(RipesSettings::getObserver(RIPES_SETTING_REWINDSTACKSIZE),
          &SettingObserver::modified, m_reverseAction, [this](const auto &) {
            m_reverseAction->setEnabled(m_vsrtlWidget->isReversible());
          });

  // Connect the global reset request signal to reset()
  connect(ProcessorHandler::get(), &ProcessorHandler::processorReset, this,
          &ProcessorTab::reset);
  connect(ProcessorHandler::get(), &ProcessorHandler::exit, this,
          &ProcessorTab::processorFinished);
  connect(ProcessorHandler::get(), &ProcessorHandler::runFinished, this,
          &ProcessorTab::runFinished);
  connect(ProcessorHandler::get(), &ProcessorHandler::stopping, this,
          &ProcessorTab::pause);

  if (m_usingTomasulo) {
    showTomasuloView();
  } else {
    showVSRTLView();
  }

  // Make processor view stretch wrt. right side tabs
  m_ui->viewSplitter->setStretchFactor(0, 1);
  m_ui->viewSplitter->setStretchFactor(1, 0);

  // Adjust sizing between register view and instruction view
  m_ui->rightBarSplitter->setStretchFactor(0, 6);
  m_ui->rightBarSplitter->setStretchFactor(1, 1);

  // Initially, no file is loaded, disable toolbuttons
  enableSimulatorControls();
}

void ProcessorTab::setupTomasuloOptionsWidget() {
  if (m_tomasuloOptionsGroup) {
    return;
  }

  m_tomasuloOptionsGroup =
      new QGroupBox("Tomasulo options", m_ui->horizontalLayoutWidget_2);
  m_tomasuloOptionsGroup->setToolTip(
      "Default Tomasulo configuration. These values can be modified and will "
      "be used by the Tomasulo simulator integration.");

  // Aumentado para que los selectores de latencias no recorten "cycles".
  m_tomasuloOptionsGroup->setMinimumWidth(430);
  m_tomasuloOptionsGroup->setMaximumWidth(560);

  m_tomasuloOptionsGroup->setSizePolicy(QSizePolicy::Preferred,
                                        QSizePolicy::MinimumExpanding);

  auto *mainLayout = new QVBoxLayout(m_tomasuloOptionsGroup);
  mainLayout->setContentsMargins(8, 6, 8, 6);
  mainLayout->setSpacing(4);

  auto *columnsLayout = new QHBoxLayout();
  columnsLayout->setSpacing(12);

  auto *stationsColumn = new QVBoxLayout();
  stationsColumn->setSpacing(4);

  auto *latenciesColumn = new QVBoxLayout();
  latenciesColumn->setSpacing(4);

  auto *stationsTitle =
      createTomasuloSectionLabel("Stations / buffers", m_tomasuloOptionsGroup);
  auto *latenciesTitle =
      createTomasuloSectionLabel("Latencies", m_tomasuloOptionsGroup);

  auto *stationsForm = new QFormLayout();
  stationsForm->setContentsMargins(0, 0, 0, 0);
  stationsForm->setSpacing(4);
  stationsForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  stationsForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  auto *latenciesForm = new QFormLayout();
  latenciesForm->setContentsMargins(0, 0, 0, 0);
  latenciesForm->setSpacing(4);
  latenciesForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  latenciesForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  m_tomasuloAddSubStations =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 32);
  m_tomasuloMultStations =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 32);
  m_tomasuloDivStations =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 32);
  m_tomasuloLoadBuffers =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 32);
  m_tomasuloStoreBuffers =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 32);

  m_tomasuloAddSubLatency =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 999, " cycles");
  m_tomasuloMultLatency =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 999, " cycles");
  m_tomasuloDivLatency =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 999, " cycles");
  m_tomasuloLoadLatency =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 999, " cycles");
  m_tomasuloStoreLatency =
      createTomasuloSpinBox(m_tomasuloOptionsGroup, 1, 999, " cycles");

  stationsForm->addRow("Add/Sub:", m_tomasuloAddSubStations);
  stationsForm->addRow("Mult:", m_tomasuloMultStations);
  stationsForm->addRow("Div:", m_tomasuloDivStations);
  stationsForm->addRow("Load:", m_tomasuloLoadBuffers);
  stationsForm->addRow("Store:", m_tomasuloStoreBuffers);

  latenciesForm->addRow("Add/Sub:", m_tomasuloAddSubLatency);
  latenciesForm->addRow("Mult:", m_tomasuloMultLatency);
  latenciesForm->addRow("Div:", m_tomasuloDivLatency);
  latenciesForm->addRow("Load:", m_tomasuloLoadLatency);
  latenciesForm->addRow("Store:", m_tomasuloStoreLatency);

  stationsColumn->addWidget(stationsTitle);
  stationsColumn->addLayout(stationsForm);
  stationsColumn->addStretch();

  latenciesColumn->addWidget(latenciesTitle);
  latenciesColumn->addLayout(latenciesForm);
  latenciesColumn->addStretch();

  columnsLayout->addLayout(stationsColumn);
  columnsLayout->addLayout(latenciesColumn);

  auto *buttonLayout = new QHBoxLayout();
  buttonLayout->setContentsMargins(0, 0, 0, 0);

  auto *defaultsButton = new QPushButton("Defaults", m_tomasuloOptionsGroup);
  defaultsButton->setToolTip("Restore the default Tomasulo configuration");
  connect(defaultsButton, &QPushButton::clicked, this,
          &ProcessorTab::resetTomasuloOptionsToDefaults);

  buttonLayout->addStretch();
  buttonLayout->addWidget(defaultsButton);

  mainLayout->addLayout(columnsLayout);
  mainLayout->addLayout(buttonLayout);

  resetTomasuloOptionsToDefaults();

  // En processortab.ui, horizontalLayout_2 contiene:
  //   0 -> Console/Memory tab widget
  //   1 -> Execution info
  // Insertamos Tomasulo options en medio:
  //   0 -> Console/Memory
  //   1 -> Tomasulo options
  //   2 -> Execution info
  m_ui->horizontalLayout_2->insertWidget(1, m_tomasuloOptionsGroup);
  m_ui->horizontalLayout_2->setStretch(0, 1);
  m_ui->horizontalLayout_2->setStretch(1, 0);
  m_ui->horizontalLayout_2->setStretch(2, 0);
}

void ProcessorTab::resetTomasuloOptionsToDefaults() {
  if (!m_tomasuloAddSubStations || !m_tomasuloMultStations ||
      !m_tomasuloDivStations || !m_tomasuloLoadBuffers ||
      !m_tomasuloStoreBuffers || !m_tomasuloAddSubLatency ||
      !m_tomasuloMultLatency || !m_tomasuloDivLatency ||
      !m_tomasuloLoadLatency || !m_tomasuloStoreLatency) {
    return;
  }

  // Default reservation stations / buffers
  m_tomasuloAddSubStations->setValue(3);
  m_tomasuloMultStations->setValue(2);
  m_tomasuloDivStations->setValue(1);
  m_tomasuloLoadBuffers->setValue(3);
  m_tomasuloStoreBuffers->setValue(3);

  // Default operation latencies
  m_tomasuloAddSubLatency->setValue(2);
  m_tomasuloMultLatency->setValue(10);
  m_tomasuloDivLatency->setValue(40);
  m_tomasuloLoadLatency->setValue(2);
  m_tomasuloStoreLatency->setValue(2);
}

void ProcessorTab::syncTomasuloWithEditor() {
  if (!m_tomasuloWidget) {
    return;
  }

  m_tomasuloWidget->loadProgramText(
      RipesSettings::value(RIPES_SETTING_SOURCECODE).toString());
}

void ProcessorTab::showTomasuloView() {
  if (!m_tomasuloWidget || !m_vsrtlWidget) {
    return;
  }

  syncTomasuloWithEditor();

  m_tomasuloWidget->show();
  m_vsrtlWidget->hide();

  if (m_tomasuloOptionsGroup) {
    m_tomasuloOptionsGroup->show();
  }

  // Tras insertar Tomasulo, el splitter queda asi:
  //   0 -> TomasuloWidget
  //   1 -> vsrtlWidget oculto
  //   2 -> zona inferior: consola/memoria/opciones/info
  m_ui->pipelinesplitter->setStretchFactor(0, 5);
  m_ui->pipelinesplitter->setStretchFactor(1, 0);
  m_ui->pipelinesplitter->setStretchFactor(2, 2);
  m_ui->pipelinesplitter->setSizes(QList<int>{520, 0, 220});
}

void ProcessorTab::showVSRTLView() {
  if (!m_tomasuloWidget || !m_vsrtlWidget) {
    return;
  }

  m_tomasuloWidget->hide();
  m_vsrtlWidget->show();

  if (m_tomasuloOptionsGroup) {
    m_tomasuloOptionsGroup->hide();
  }

  // Modo normal de Ripes:
  //   0 -> TomasuloWidget oculto
  //   1 -> vsrtlWidget
  //   2 -> zona inferior: consola/memoria/info
  m_ui->pipelinesplitter->setStretchFactor(0, 0);
  m_ui->pipelinesplitter->setStretchFactor(1, 5);
  m_ui->pipelinesplitter->setStretchFactor(2, 2);
  m_ui->pipelinesplitter->setSizes(QList<int>{0, 520, 220});

  fitToScreen();
}

void ProcessorTab::loadLayout(const Layout &layout) {
  if (layout.name.isEmpty() || layout.file.isEmpty())
    return; // Not a valid layout

  if (layout.stageLabelPositions.size() !=
      ProcessorHandler::getProcessor()->structure().numStages()) {
    Q_ASSERT(false &&
             "A stage label position must be specified for each stage");
  }

  // cereal expects the archive file to be present standalone on disk, and
  // available through an ifstream. Copy the resource layout file (bundled
  // within the binary as a Qt resource) to a temporary file, for loading the
  // layout.
  const auto &layoutResourceFilename = layout.file;
  QFile layoutResourceFile(layoutResourceFilename);
  QTemporaryFile *tmpLayoutFile =
      QTemporaryFile::createNativeFile(layoutResourceFile);
  if (!tmpLayoutFile->open()) {
    QMessageBox::warning(this, "Error",
                         "Could not create temporary layout file");
    return;
  }

  m_vsrtlWidget->getTopLevelComponent()->loadLayoutFile(
      tmpLayoutFile->fileName());
  tmpLayoutFile->remove();

  // Adjust stage label positions
  const auto &parent = m_stageInstructionLabels.at({0, 0})->parentItem();
  for (auto sid : ProcessorHandler::getProcessor()->structure().stageIt()) {
    auto &label = m_stageInstructionLabels.at(sid);
    QFontMetrics metrics(label->font());
    label->setPos(parent->boundingRect().width() *
                      layout.stageLabelPositions.at(sid).x(),
                  metrics.height() * layout.stageLabelPositions.at(sid).y());
  }
}

void ProcessorTab::setupSimulatorActions(QToolBar *controlToolbar) {
  const QIcon processorIcon = QIcon(":/icons/cpu.svg");
  m_selectProcessorAction =
      new QAction(processorIcon, "Select processor", this);
  connect(m_selectProcessorAction, &QAction::triggered, this,
          &ProcessorTab::processorSelection);
  controlToolbar->addAction(m_selectProcessorAction);

  const QIcon cacheIcon = QIcon(":/icons/hierarchy_test.svg");
  m_selectCacheAction = new QAction(cacheIcon, "Select cache", this);
  connect(m_selectCacheAction, &QAction::triggered, this,
          &ProcessorTab::cacheSelection);
  controlToolbar->addAction(m_selectCacheAction);
  controlToolbar->addSeparator();

  const QIcon resetIcon = QIcon(":/icons/reset.svg");
  m_resetAction = new QAction(resetIcon, "Reset (F3)", this);
  connect(m_resetAction, &QAction::triggered, this, [this] {
    RipesSettings::getObserver(RIPES_GLOBALSIGNAL_REQRESET)->trigger();
  });
  m_resetAction->setShortcut(QKeySequence("F3"));
  m_resetAction->setToolTip("Reset the simulator (F3)");
  controlToolbar->addAction(m_resetAction);

  const QIcon reverseIcon = QIcon(":/icons/reverse.svg");
  m_reverseAction = new QAction(reverseIcon, "Reverse (F4)", this);
  connect(m_reverseAction, &QAction::triggered, this, &ProcessorTab::reverse);
  m_reverseAction->setShortcut(QKeySequence("F4"));
  m_reverseAction->setToolTip("Undo a clock cycle (F4)");
  controlToolbar->addAction(m_reverseAction);

  const QIcon clockIcon = QIcon(":/icons/step.svg");
  m_clockAction = new QAction(clockIcon, "Clock (F5)", this);
  connect(m_clockAction, &QAction::triggered, this,
          [] { ProcessorHandler::clock(); });
  m_clockAction->setShortcut(QKeySequence("F5"));
  m_clockAction->setToolTip("Clock the circuit (F5)");
  controlToolbar->addAction(m_clockAction);

  m_autoClockTimer = new QTimer(this);
  connect(m_autoClockTimer, &QTimer::timeout, this,
          [this] { autoClockTimeout(); });

  const QIcon startAutoClockIcon = QIcon(":/icons/step-clock.svg");
  m_autoClockAction = new QAction(startAutoClockIcon, "Auto clock (F6)", this);
  m_autoClockAction->setShortcut(QKeySequence("F6"));
  m_autoClockAction->setToolTip(
      "Clock the circuit with the selected frequency (F6)");
  m_autoClockAction->setCheckable(true);
  m_autoClockAction->setChecked(false);
  connect(m_autoClockAction, &QAction::toggled, this, &ProcessorTab::autoClock);
  controlToolbar->addAction(m_autoClockAction);

  m_autoClockInterval = new QSpinBox(this);
  m_autoClockInterval->setRange(1, 10000);
  m_autoClockInterval->setSuffix(" ms");
  m_autoClockInterval->setToolTip("Auto clock interval");
  connect(m_autoClockInterval, qOverload<int>(&QSpinBox::valueChanged), this,
          [this](int msec) {
            RipesSettings::setValue(RIPES_SETTING_AUTOCLOCK_INTERVAL, msec);
            m_autoClockTimer->setInterval(msec);
          });
  m_autoClockInterval->setValue(
      RipesSettings::value(RIPES_SETTING_AUTOCLOCK_INTERVAL).toInt());
  controlToolbar->addWidget(m_autoClockInterval);

  const QIcon runIcon = QIcon(":/icons/run.svg");
  m_runAction = new QAction(runIcon, "Run (F8)", this);
  m_runAction->setShortcut(QKeySequence("F8"));
  m_runAction->setCheckable(true);
  m_runAction->setChecked(false);
  m_runAction->setToolTip(
      "Execute simulator without updating UI (fast execution) (F8).\n Running "
      "will stop once the program exits or a "
      "breakpoint is hit.");
  connect(m_runAction, &QAction::toggled, this, &ProcessorTab::run);
  controlToolbar->addAction(m_runAction);

  // Setup processor-tab only actions
  m_displayValuesAction = new QAction("Show processor signal values", this);
  m_displayValuesAction->setCheckable(true);
  connect(m_displayValuesAction, &QAction::toggled, m_vsrtlWidget,
          [this](bool checked) {
            RipesSettings::setValue(RIPES_SETTING_SHOWSIGNALS,
                                    QVariant::fromValue(checked));
            m_vsrtlWidget->setOutputPortValuesVisible(checked);
          });
  m_displayValuesAction->setChecked(
      RipesSettings::value(RIPES_SETTING_SHOWSIGNALS).toBool());

  const QIcon tableIcon = QIcon(":/icons/spreadsheet.svg");
  m_pipelineDiagramAction =
      new QAction(tableIcon, "Show pipeline diagram", this);
  connect(m_pipelineDiagramAction, &QAction::triggered, this,
          &ProcessorTab::showPipelineDiagram);
  m_toolbar->addAction(m_pipelineDiagramAction);

  m_darkmodeAction = new QAction("Processor darkmode", this);
  m_darkmodeAction->setCheckable(true);
  connect(m_darkmodeAction, &QAction::toggled, m_vsrtlWidget,
          [this](bool checked) {
            RipesSettings::setValue(RIPES_SETTING_DARKMODE,
                                    QVariant::fromValue(checked));
            m_vsrtlWidget->setDarkmode(checked);
          });
  m_darkmodeAction->setChecked(
      RipesSettings::value(RIPES_SETTING_DARKMODE).toBool());
}

void ProcessorTab::updateStatistics() {
  static auto lastUpdateTime = std::chrono::system_clock::now();
  static long long lastCycleCount =
      ProcessorHandler::getProcessor()->getCycleCount();

  const auto timeNow = std::chrono::system_clock::now();
  const auto cycleCount = ProcessorHandler::getProcessor()->getCycleCount();
  const auto instrsRetired =
      ProcessorHandler::getProcessor()->getInstructionsRetired();
  const auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
                            timeNow - lastUpdateTime)
                            .count() /
                        1000.0; // in seconds
  const auto cycleDiff = cycleCount - lastCycleCount;

  // Cycle count
  m_ui->cycleCount->setText(QString::number(cycleCount));
  // Instructions retired
  m_ui->instructionsRetired->setText(QString::number(instrsRetired));
  QString cpiText, ipcText;
  if (cycleCount != 0 && instrsRetired != 0) {
    const double cpi =
        static_cast<double>(cycleCount) / static_cast<double>(instrsRetired);
    const double ipc = 1 / cpi;
    cpiText = QString::number(cpi, 'g', 3);
    ipcText = QString::number(ipc, 'g', 3);
  }
  // CPI & IPC
  m_ui->cpi->setText(cpiText);
  m_ui->ipc->setText(ipcText);

  // Clock rate
  const double clockRate = static_cast<double>(cycleDiff) / timeDiff;
  m_ui->clockRate->setText(convertToSIUnits(clockRate) + "Hz");

  // Record timestamp values
  lastUpdateTime = timeNow;
  lastCycleCount = cycleCount;
}

void ProcessorTab::pause() {
  m_autoClockAction->setChecked(false);
  m_runAction->setChecked(false);
  m_reverseAction->setEnabled(m_vsrtlWidget->isReversible());
}

void ProcessorTab::fitToScreen() { m_vsrtlWidget->zoomToFit(); }

void ProcessorTab::loadProcessorToWidget(const Layout *layout) {
  const bool doPlaceAndRoute = layout != nullptr;
  ProcessorHandler::loadProcessorToWidget(m_vsrtlWidget, doPlaceAndRoute);

  // Construct stage instruction labels
  auto *topLevelComponent = m_vsrtlWidget->getTopLevelComponent();

  m_stageInstructionLabels.clear();
  for (auto laneIt : ProcessorHandler::getProcessor()->structure()) {
    for (unsigned stageIdx = 0; stageIdx < laneIt.second; stageIdx++) {
      StageIndex sid = {laneIt.first, stageIdx};
      auto *stagelabel = new vsrtl::Label(topLevelComponent, "-");
      stagelabel->setPointSize(14);
      m_stageInstructionLabels[sid] = stagelabel;
    }
  }
  if (layout != nullptr) {
    loadLayout(*layout);
  }
  updateInstructionLabels();
  fitToScreen();
}

void ProcessorTab::processorSelection() {
  m_autoClockAction->setChecked(false);

  ProcessorSelectionDialog diag;
  diag.setTomasuloSelected(m_usingTomasulo);

  if (diag.exec()) {
    if (diag.isTomasuloSelected()) {
      m_usingTomasulo = true;
      showTomasuloView();
      enableSimulatorControls();
      return;
    }

    m_usingTomasulo = false;
    showVSRTLView();

    // New processor model was selected
    m_vsrtlWidget->clearDesign();
    m_stageInstructionLabels.clear();
    ProcessorHandler::selectProcessor(diag.getSelectedId(),
                                      diag.getEnabledExtensions(),
                                      diag.getRegisterInitialization());

    // Store selected layout index
    const auto &layouts =
        ProcessorRegistry::getDescription(diag.getSelectedId()).layouts;
    if (auto *layout = diag.getSelectedLayout()) {
      auto layoutIter = std::find(layouts.begin(), layouts.end(), *layout);
      Q_ASSERT(layoutIter != layouts.end());
      const long layoutIndex = std::distance(layouts.begin(), layoutIter);
      RipesSettings::setValue(RIPES_SETTING_PROCESSOR_LAYOUT_ID,
                              static_cast<int>(layoutIndex));
    }

    if (ProcessorHandler::isVSRTLProcessor()) {
      loadProcessorToWidget(diag.getSelectedLayout());
    }
    updateInstructionModel();

    // Retrigger value display action if enabled
    if (m_displayValuesAction->isChecked()) {
      m_vsrtlWidget->setOutputPortValuesVisible(true);
    }

    enableSimulatorControls();
  }
}

void ProcessorTab::cacheSelection() {
  CacheSelectionDialog dialog;
  if (dialog.exec() == QDialog::Accepted) {
    CacheConfigType selectedType = dialog.getSelectedCacheType();
    RipesSettings::setValue("CacheTypeSelected",
                            static_cast<int>(selectedType));
    qDebug() << "Cache type selected:" << static_cast<int>(selectedType);

    emit cacheConfigurationChanged();
  }
}

void ProcessorTab::updateInstructionModel() {
  auto *oldModel = m_instrModel;
  m_instrModel = new InstructionModel(this);

  // Update the instruction view according to the newly created model
  m_ui->instructionView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_ui->instructionView->setModel(m_instrModel);

  // Only the instruction column should stretch
  m_ui->instructionView->horizontalHeader()->setMinimumSectionSize(1);
  m_ui->instructionView->horizontalHeader()->setSectionResizeMode(
      InstructionModel::Breakpoint, QHeaderView::ResizeToContents);
  m_ui->instructionView->horizontalHeader()->setSectionResizeMode(
      InstructionModel::PC, QHeaderView::ResizeToContents);
  // The "stage" section is _NOT_ resized to contents. Resize to contents is
  // very slow if # of items in the model is large and the contents of the rows
  // change frequently.
  m_ui->instructionView->horizontalHeader()->setSectionResizeMode(
      InstructionModel::Stage, QHeaderView::Interactive);
  auto ivfm = QFontMetrics(m_ui->instructionView->font());
  m_ui->instructionView->horizontalHeader()->resizeSection(
      InstructionModel::Stage,
      ivfm.horizontalAdvance(m_instrModel
                                 ->headerData(InstructionModel::Stage,
                                              Qt::Horizontal, Qt::DisplayRole)
                                 .toString()) *
          1.25);
  m_ui->instructionView->horizontalHeader()->setSectionResizeMode(
      InstructionModel::Instruction, QHeaderView::Stretch);
  // Make the instruction view follow the instruction which is currently present
  // in the first stage of the
  connect(m_instrModel, &InstructionModel::firstStageInstrChanged, this,
          &ProcessorTab::setInstructionViewCenterRow);

  if (oldModel) {
    delete oldModel;
  }
}

void ProcessorTab::restart() {
  // Invoked when changes to binary simulation file has been made
  enableSimulatorControls();
}

ProcessorTab::~ProcessorTab() { delete m_ui; }

void ProcessorTab::processorFinished() {
  // Disallow further clocking of the circuit
  m_clockAction->setEnabled(false);
  m_autoClockAction->setChecked(false);
  m_autoClockAction->setEnabled(false);
  m_runAction->setEnabled(false);
  m_runAction->setChecked(false);
}

void ProcessorTab::enableSimulatorControls() {
  m_clockAction->setEnabled(true);
  m_autoClockAction->setEnabled(true);
  m_runAction->setEnabled(true);
  m_reverseAction->setEnabled(m_vsrtlWidget->isReversible());
  m_resetAction->setEnabled(true);
  m_pipelineDiagramAction->setEnabled(true);
}

void ProcessorTab::updateInstructionLabels() {
  const auto &proc = ProcessorHandler::getProcessor();
  for (auto sid : ProcessorHandler::getProcessor()->structure().stageIt()) {
    if (!m_stageInstructionLabels.count(sid))
      continue;
    const auto stageInfo = proc->stageInfo(sid);
    auto &instrLabel = m_stageInstructionLabels.at(sid);
    QString instrString;
    if (stageInfo.state != StageInfo::State::None) {
      /* clang-format off */
            switch (stageInfo.state) {
                case StageInfo::State::Flushed: instrString = "nop (flush)"; break;
                case StageInfo::State::Stalled: instrString = "nop (stall)"; break;
                case StageInfo::State::WayHazard: if(stageInfo.stage_valid) {instrString = "nop (way hazard)";} break;
                case StageInfo::State::Unused: instrString = "nop (unused)"; break;
                case StageInfo::State::None: Q_UNREACHABLE();
            }
      /* clang-format on */
      instrLabel->forceDefaultTextColor(Qt::red);
    } else if (stageInfo.stage_valid) {
      instrString = ProcessorHandler::disassembleInstr(stageInfo.pc);
      instrLabel->clearForcedDefaultTextColor();
    }
    instrLabel->setText(instrString);
  }
}

void ProcessorTab::reset() {
  m_autoClockAction->setChecked(false);
  enableSimulatorControls();
  SystemIO::printString("\n");
}

void ProcessorTab::setInstructionViewCenterRow(int row) {
  const auto view = m_ui->instructionView;
  const auto rect = view->rect();
  int rowTop = view->indexAt(rect.topLeft()).row();
  int rowBot = view->indexAt(rect.bottomLeft()).row();
  rowBot = rowBot < 0 ? m_instrModel->rowCount() : rowBot;

  const int nItemsVisible = rowBot - rowTop;

  // move scrollbar if if is not visible
  if (row <= rowTop || row >= rowBot) {
    auto scrollbar = view->verticalScrollBar();
    scrollbar->setValue(row - nItemsVisible / 2);
  }
}

void ProcessorTab::runFinished() {
  pause();
  ProcessorHandler::checkProcessorFinished();
  m_vsrtlWidget->sync();
  m_statUpdateTimer->stop();
}

void ProcessorTab::autoClockTimeout() {
  if (ProcessorHandler::checkBreakpoint())
    return;
  ProcessorHandler::clock();
}

void ProcessorTab::autoClock(bool state) {
  const QIcon startAutoClockIcon = QIcon(":/icons/step-clock.svg");
  const QIcon stopAutoTimerIcon = QIcon(":/icons/stop-clock.svg");
  if (!state) {
    m_autoClockTimer->stop();
    m_autoClockAction->setIcon(startAutoClockIcon);
  } else {
    // Always clock the processor to start with. Afterwards, run
    // autoClockTimeout() which will check if the processor is at a breakpoint.
    // This is to circumvent some annoying cross-thread, eventloop,
    // race-condition-y state setting wrt. when exactly a breakpoint is hit.
    ProcessorHandler::clock();
    m_autoClockTimer->start();
    m_autoClockAction->setIcon(stopAutoTimerIcon);
  }

  // Enable/disable all other actions
  m_selectProcessorAction->setEnabled(!state);
  m_clockAction->setEnabled(!state);
  m_reverseAction->setEnabled(!state);
  m_resetAction->setEnabled(!state);
  m_displayValuesAction->setEnabled(!state);
  m_pipelineDiagramAction->setEnabled(!state);
  m_runAction->setEnabled(!state);

  if (m_tomasuloOptionsGroup) {
    m_tomasuloOptionsGroup->setEnabled(!state);
  }
}

void ProcessorTab::run(bool state) {
  // Stop any currently exeuting auto-clocking
  if (m_autoClockAction->isChecked()) {
    m_autoClockAction->setChecked(false);
  }
  if (state) {
    ProcessorHandler::run();
    m_statUpdateTimer->start();
  } else {
    ProcessorHandler::stopRun();
    m_statUpdateTimer->stop();
  }

  // Enable/Disable all actions based on whether the processor is running.
  m_selectProcessorAction->setEnabled(!state);
  m_clockAction->setEnabled(!state);
  m_autoClockAction->setEnabled(!state);
  m_reverseAction->setEnabled(!state);
  m_resetAction->setEnabled(!state);
  m_displayValuesAction->setEnabled(!state);
  m_pipelineDiagramAction->setEnabled(!state);

  // Disable widgets which are not updated when running the processor
  m_vsrtlWidget->setEnabled(!state);
  m_ui->registerContainerWidget->setEnabled(!state);
  m_ui->instructionView->setEnabled(!state);

  if (m_tomasuloOptionsGroup) {
    m_tomasuloOptionsGroup->setEnabled(!state);
  }
}

void ProcessorTab::reverse() {
  m_vsrtlWidget->reverse();
  enableSimulatorControls();
}

void ProcessorTab::showPipelineDiagram() {
  auto w = PipelineDiagramWidget(m_stageModel);
  w.exec();
}

} // namespace Ripes