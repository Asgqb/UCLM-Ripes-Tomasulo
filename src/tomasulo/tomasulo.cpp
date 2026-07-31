#include "tomasulo.h"

#include <algorithm>
#include <vector>

#include <QAbstractItemView>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QList>
#include <QMap>
#include <QSizePolicy>
#include <QSplitter>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace Ripes {

static QTableWidgetItem *makeTableItem(const QString &text) {
  auto *item = new QTableWidgetItem(text);
  item->setFlags(Qt::ItemIsEnabled);
  return item;
}

static QString detectRegisterPrefix(
    const std::vector<TomasuloSim::TomasuloRegisterResultStatus> &registers) {
  for (const auto &reg : registers) {
    const QString name = reg.registerName.trimmed().toUpper();

    if (name.startsWith("X")) {
      return "X";
    }
  }

  for (const auto &reg : registers) {
    const QString name = reg.registerName.trimmed().toUpper();

    if (name.startsWith("F")) {
      return "F";
    }
  }

  return "F";
}

static void fillRegisterTable(QTableWidget *table,
                              const QMap<QString, QString> &qiByRegister,
                              const QString &prefix, int firstRegister,
                              int columnCount, int step) {
  table->clear();
  table->setRowCount(1);
  table->setColumnCount(columnCount);
  table->setVerticalHeaderLabels({"Qi"});

  QStringList headers;

  for (int col = 0; col < columnCount; ++col) {
    const int regNumber = firstRegister + col * step;
    const QString regName = prefix + QString::number(regNumber);

    headers << regName;
    table->setItem(0, col, makeTableItem(qiByRegister.value(regName)));
  }

  table->setHorizontalHeaderLabels(headers);
}

TomasuloWidget::TomasuloWidget(QWidget *parent)
    : QWidget(parent), m_engine(std::make_unique<TomasuloSim::TomasuloEngine>()) {
  setupUi();
  loadSnapshot(m_engine->snapshot());
}

TomasuloWidget::~TomasuloWidget() = default;

void TomasuloWidget::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(4);

  auto *mainSplitter = new QSplitter(Qt::Vertical, this);
  mainSplitter->setChildrenCollapsible(false);
  mainSplitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *topContainer = new QWidget(mainSplitter);
  topContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *topLayout = new QHBoxLayout(topContainer);
  topLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->setSpacing(6);

  auto *instructionGroup = new QGroupBox("Estado instruccion", topContainer);
  instructionGroup->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);

  auto *instructionLayout = new QVBoxLayout(instructionGroup);
  instructionLayout->setContentsMargins(4, 4, 4, 4);
  instructionLayout->setSpacing(2);

  setupInstructionTable();
  instructionLayout->addWidget(m_instructionTable);

  auto *reservationGroup =
      new QGroupBox("Estado de las estaciones de reserva", topContainer);
  reservationGroup->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);

  auto *reservationLayout = new QVBoxLayout(reservationGroup);
  reservationLayout->setContentsMargins(4, 4, 4, 4);
  reservationLayout->setSpacing(2);

  setupReservationStationsTable();
  reservationLayout->addWidget(m_reservationTable);

  topLayout->addWidget(instructionGroup, 1);
  topLayout->addWidget(reservationGroup, 2);

  auto *registerGroup =
      new QGroupBox("Estado de los registros resultado", mainSplitter);
  registerGroup->setMinimumHeight(125);
  registerGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  auto *registerLayout = new QVBoxLayout(registerGroup);
  registerLayout->setContentsMargins(4, 4, 4, 4);
  registerLayout->setSpacing(4);

  setupRegisterResultTables();

  registerLayout->addWidget(m_registerResultTable1);
  registerLayout->addWidget(m_registerResultTable2);

  mainSplitter->addWidget(topContainer);
  mainSplitter->addWidget(registerGroup);

  mainSplitter->setStretchFactor(0, 4);
  mainSplitter->setStretchFactor(1, 1);
  mainSplitter->setSizes(QList<int>{420, 135});

  mainLayout->addWidget(mainSplitter);
}

void TomasuloWidget::setupInstructionTable() {
  m_instructionTable = new QTableWidget(this);

  m_instructionTable->setColumnCount(4);
  m_instructionTable->setHorizontalHeaderLabels(
      {"Instruccion", "Emision", "Ejecucion", "Escritura"});

  configureTable(m_instructionTable);
}

void TomasuloWidget::setupReservationStationsTable() {
  m_reservationTable = new QTableWidget(this);

  m_reservationTable->setColumnCount(8);
  m_reservationTable->setHorizontalHeaderLabels(
      {"Nombre", "Ocupada", "Operacion", "Vj", "Vk", "Qj", "Qk", "A"});

  configureTable(m_reservationTable);
}

void TomasuloWidget::setupRegisterResultTables() {
  m_registerResultTable1 = new QTableWidget(this);
  m_registerResultTable1->setColumnCount(8);
  m_registerResultTable1->setRowCount(1);
  m_registerResultTable1->setHorizontalHeaderLabels(
      {"F0", "F2", "F4", "F6", "F8", "F10", "F12", "F14"});
  m_registerResultTable1->setVerticalHeaderLabels({"Qi"});
  configureCompactRegisterTable(m_registerResultTable1);

  m_registerResultTable2 = new QTableWidget(this);
  m_registerResultTable2->setColumnCount(8);
  m_registerResultTable2->setRowCount(1);
  m_registerResultTable2->setHorizontalHeaderLabels(
      {"F16", "F18", "F20", "F22", "F24", "F26", "F28", "F30"});
  m_registerResultTable2->setVerticalHeaderLabels({"Qi"});
  configureCompactRegisterTable(m_registerResultTable2);
}

void TomasuloWidget::configureTable(QTableWidget *table, int rowHeight) {
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setFocusPolicy(Qt::NoFocus);
  table->setAlternatingRowColors(false);

  table->horizontalHeader()->setStretchLastSection(false);
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table->horizontalHeader()->setFixedHeight(22);

  table->verticalHeader()->setVisible(false);
  table->verticalHeader()->setDefaultSectionSize(rowHeight);

  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TomasuloWidget::configureCompactRegisterTable(QTableWidget *table) {
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setFocusPolicy(Qt::NoFocus);

  table->horizontalHeader()->setFixedHeight(22);
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  table->verticalHeader()->setFixedWidth(32);
  table->verticalHeader()->setDefaultSectionSize(22);

  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  table->setFixedHeight(54);
}

void TomasuloWidget::loadProgramText(const QString &programText) {
  resetSimulation(programText, TomasuloSim::TomasuloEngine::defaultConfig());
}


void TomasuloWidget::resetSimulation(const QString &programText,
                                     const TomasuloSim::Config &config) {
  m_programText = programText;
  m_config = config;

  if (!m_engine) {
    m_engine = std::make_unique<TomasuloSim::TomasuloEngine>();
  }

  m_engine->loadProgramText(m_programText, m_config);

  m_history.clear();
  m_history.push_back(m_engine->snapshot());
  m_historyIndex = 0;

  loadSnapshot(m_history[m_historyIndex]);
}

const TomasuloSim::TomasuloSnapshot *
TomasuloWidget::currentHistorySnapshot() const {
  if (m_history.empty() || m_historyIndex >= m_history.size()) {
    return nullptr;
  }

  return &m_history[m_historyIndex];
}

void TomasuloWidget::rebuildEngineToHistoryIndex() {
  if (!m_engine) {
    m_engine = std::make_unique<TomasuloSim::TomasuloEngine>();
  }

  m_engine->loadProgramText(m_programText, m_config);

  for (std::size_t i = 0; i < m_historyIndex && !m_engine->isFinished(); ++i) {
    m_engine->step();
  }
}

void TomasuloWidget::clock() {
  if (!m_engine) {
    return;
  }

  // Si estamos viendo un ciclo antiguo y ejecutamos desde ahi,
  // reconstruimos el motor hasta ese ciclo y descartamos el futuro.
  if (m_historyIndex + 1 < m_history.size()) {
    rebuildEngineToHistoryIndex();
    m_history.resize(m_historyIndex + 1);
  }

  if (!m_engine->isFinished()) {
    m_engine->step();

    m_history.push_back(m_engine->snapshot());
    m_historyIndex = m_history.size() - 1;
  }

  if (const auto *snapshot = currentHistorySnapshot()) {
    loadSnapshot(*snapshot);
  }
}

void TomasuloWidget::stepBack() {
  if (!canStepBack()) {
    return;
  }

  --m_historyIndex;
  loadSnapshot(m_history[m_historyIndex]);
}

void TomasuloWidget::stepForward() {
  if (canStepForward()) {
    ++m_historyIndex;
    loadSnapshot(m_history[m_historyIndex]);
    return;
  }

  clock();
}

bool TomasuloWidget::canStepBack() const {
  return !m_history.empty() && m_historyIndex > 0;
}

bool TomasuloWidget::canStepForward() const {
  return !m_history.empty() && m_historyIndex + 1 < m_history.size();
}

void TomasuloWidget::runToCompletion() {
  if (!m_engine) {
    return;
  }

  if (m_historyIndex + 1 < m_history.size()) {
    rebuildEngineToHistoryIndex();
    m_history.resize(m_historyIndex + 1);
  }

  std::uint64_t guard = 0;
  constexpr std::uint64_t maxCycles = 100000;

  while (!m_engine->isFinished() && guard < maxCycles) {
    m_engine->step();

    m_history.push_back(m_engine->snapshot());
    m_historyIndex = m_history.size() - 1;

    ++guard;
  }

  if (const auto *snapshot = currentHistorySnapshot()) {
    loadSnapshot(*snapshot);
  }
}

std::uint64_t TomasuloWidget::currentCycle() const {
  if (const auto *snapshot = currentHistorySnapshot()) {
    return snapshot->cycle;
  }

  if (!m_engine) {
    return 0;
  }

  return m_engine->snapshot().cycle;
}

std::uint64_t TomasuloWidget::instructionsRetired() const {
  if (const auto *snapshot = currentHistorySnapshot()) {
    return snapshot->instructionsRetired;
  }

  if (!m_engine) {
    return 0;
  }

  return m_engine->snapshot().instructionsRetired;
}

bool TomasuloWidget::isFinished() const {
  if (const auto *snapshot = currentHistorySnapshot()) {
    return snapshot->finished;
  }

  return m_engine && m_engine->isFinished();
}
void TomasuloWidget::clearRegisterTables() {
  QMap<QString, QString> emptyValues;

  fillRegisterTable(m_registerResultTable1, emptyValues, "F", 0, 8, 2);
  fillRegisterTable(m_registerResultTable2, emptyValues, "F", 16, 8, 2);
}

void TomasuloWidget::loadSnapshot(
    const TomasuloSim::TomasuloSnapshot &snapshot) {
  m_instructionTable->clearContents();
  m_instructionTable->setRowCount(
      static_cast<int>(snapshot.instructions.size()));

  for (int row = 0; row < static_cast<int>(snapshot.instructions.size());
       ++row) {
    const auto &instruction = snapshot.instructions.at(row);

    m_instructionTable->setItem(row, 0, makeTableItem(instruction.instruction));
    m_instructionTable->setItem(row, 1, makeTableItem(instruction.issueCycle));
    m_instructionTable->setItem(row, 2,
                                makeTableItem(instruction.executionCycles));
    m_instructionTable->setItem(row, 3,
                                makeTableItem(instruction.writeBackCycle));
  }

  m_reservationTable->clearContents();
  m_reservationTable->setRowCount(
      static_cast<int>(snapshot.reservationStations.size()));

  for (int row = 0; row < static_cast<int>(snapshot.reservationStations.size());
       ++row) {
    const auto &station = snapshot.reservationStations.at(row);

    m_reservationTable->setItem(row, 0, makeTableItem(station.name));
    m_reservationTable->setItem(row, 1, makeTableItem(station.busy));
    m_reservationTable->setItem(row, 2, makeTableItem(station.operation));
    m_reservationTable->setItem(row, 3, makeTableItem(station.vj));
    m_reservationTable->setItem(row, 4, makeTableItem(station.vk));
    m_reservationTable->setItem(row, 5, makeTableItem(station.qj));
    m_reservationTable->setItem(row, 6, makeTableItem(station.qk));
    m_reservationTable->setItem(row, 7, makeTableItem(station.address));
  }

  QMap<QString, QString> qiByRegister;

  for (const auto &reg : snapshot.registers) {
    qiByRegister.insert(reg.registerName.trimmed().toUpper(), reg.qi);
  }

  const QString prefix = detectRegisterPrefix(snapshot.registers);

  if (prefix == "X") {
    fillRegisterTable(m_registerResultTable1, qiByRegister, "X", 0, 16, 1);
    fillRegisterTable(m_registerResultTable2, qiByRegister, "X", 16, 16, 1);
  } else {
    fillRegisterTable(m_registerResultTable1, qiByRegister, "F", 0, 8, 2);
    fillRegisterTable(m_registerResultTable2, qiByRegister, "F", 16, 8, 2);
  }

  m_registerResultTable2->setVisible(true);

  m_instructionTable->clearSelection();
  m_instructionTable->setCurrentItem(nullptr);

  m_reservationTable->clearSelection();
  m_reservationTable->setCurrentItem(nullptr);

  m_registerResultTable1->clearSelection();
  m_registerResultTable1->setCurrentItem(nullptr);

  m_registerResultTable2->clearSelection();
  m_registerResultTable2->setCurrentItem(nullptr);
}

} // namespace Ripes

