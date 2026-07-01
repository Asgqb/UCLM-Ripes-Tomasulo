#include "tomasulo.h"

#include <QAbstractItemView>
#include <QFont>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace Ripes {

static QTableWidgetItem *makeTableItem(const QString &text) {
  auto *item = new QTableWidgetItem(text);
  item->setFlags(Qt::ItemIsEnabled);
  return item;
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

  auto *topLayout = new QHBoxLayout();
  topLayout->setSpacing(6);

  auto *instructionGroup = new QGroupBox("Estado instruccion", this);
  auto *instructionLayout = new QVBoxLayout(instructionGroup);
  instructionLayout->setContentsMargins(4, 4, 4, 4);
  instructionLayout->setSpacing(2);

  setupInstructionTable();
  instructionLayout->addWidget(m_instructionTable);

  auto *reservationGroup =
      new QGroupBox("Estado de las estaciones de reserva", this);
  auto *reservationLayout = new QVBoxLayout(reservationGroup);
  reservationLayout->setContentsMargins(4, 4, 4, 4);
  reservationLayout->setSpacing(2);

  setupReservationStationsTable();
  reservationLayout->addWidget(m_reservationTable);

  topLayout->addWidget(instructionGroup, 1);
  topLayout->addWidget(reservationGroup, 2);

  auto *registerGroup =
      new QGroupBox("Estado de los registros resultado", this);
  auto *registerLayout = new QVBoxLayout(registerGroup);
  registerLayout->setContentsMargins(4, 4, 4, 4);
  registerLayout->setSpacing(4);

  setupRegisterResultTables();

  registerLayout->addWidget(m_registerResultTable1);
  registerLayout->addWidget(m_registerResultTable2);

  mainLayout->addLayout(topLayout, 1);
  mainLayout->addWidget(registerGroup, 0);
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
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  table->setFixedHeight(54);
}

void TomasuloWidget::loadProgramText(const QString &programText) {
  resetSimulation(programText, TomasuloSim::TomasuloEngine::defaultConfig());
}

void TomasuloWidget::resetSimulation(const QString &programText,
                                     const TomasuloSim::Config &config) {
  if (!m_engine) {
    m_engine = std::make_unique<TomasuloSim::TomasuloEngine>();
  }

  m_engine->loadProgramText(programText, config);
  loadSnapshot(m_engine->snapshot());
}

void TomasuloWidget::clock() {
  if (!m_engine) {
    return;
  }

  m_engine->step();
  loadSnapshot(m_engine->snapshot());
}

void TomasuloWidget::runToCompletion() {
  if (!m_engine) {
    return;
  }

  m_engine->runToCompletion();
  loadSnapshot(m_engine->snapshot());
}

std::uint64_t TomasuloWidget::currentCycle() const {
  if (!m_engine) {
    return 0;
  }

  return m_engine->snapshot().cycle;
}

std::uint64_t TomasuloWidget::instructionsRetired() const {
  if (!m_engine) {
    return 0;
  }

  return m_engine->snapshot().instructionsRetired;
}
bool TomasuloWidget::isFinished() const {
  return m_engine && m_engine->isFinished();
}

void TomasuloWidget::clearRegisterTables() {
  for (int col = 0; col < m_registerResultTable1->columnCount(); ++col) {
    m_registerResultTable1->setItem(0, col, makeTableItem(""));
  }

  for (int col = 0; col < m_registerResultTable2->columnCount(); ++col) {
    m_registerResultTable2->setItem(0, col, makeTableItem(""));
  }
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

  clearRegisterTables();

  for (const auto &reg : snapshot.registers) {
    bool ok = false;
    const int number = reg.registerName.mid(1).toInt(&ok);

    if (!ok) {
      continue;
    }

    if (number >= 0 && number <= 14 && number % 2 == 0) {
      const int col = number / 2;
      m_registerResultTable1->setItem(0, col, makeTableItem(reg.qi));
    } else if (number >= 16 && number <= 30 && number % 2 == 0) {
      const int col = (number - 16) / 2;
      m_registerResultTable2->setItem(0, col, makeTableItem(reg.qi));
    }
  }

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


