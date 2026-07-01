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

  // Evita que las celdas puedan seleccionarse visualmente en azul.
  item->setFlags(Qt::ItemIsEnabled);

  return item;
}

TomasuloWidget::TomasuloWidget(QWidget *parent) : QWidget(parent) {
  setupUi();
}

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

  configureTable(m_instructionTable, 24);

  m_instructionTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_instructionTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  m_instructionTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Interactive);

  m_instructionTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  m_instructionTable->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  m_instructionTable->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  m_instructionTable->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);

  m_instructionTable->horizontalHeader()->setMinimumSectionSize(55);
  m_instructionTable->setMinimumHeight(230);
}

void TomasuloWidget::setupReservationStationsTable() {
  m_reservationTable = new QTableWidget(this);

  m_reservationTable->setColumnCount(8);
  m_reservationTable->setHorizontalHeaderLabels(
      {"Nombre", "Ocupada", "Operacion", "Vj", "Vk", "Qj", "Qk", "A"});

  configureTable(m_reservationTable, 24);

  m_reservationTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  m_reservationTable->horizontalHeader()->setMinimumSectionSize(55);
  m_reservationTable->horizontalHeader()->setStretchLastSection(false);

  m_reservationTable->setMinimumHeight(230);

  const QStringList stationNames = {"Add0",  "Add1",  "Add2",  "Mult0",
                                    "Mult1", "Divide0", "Load0", "Load1",
                                    "Load2", "Store0", "Store1", "Store2"};

  m_reservationTable->setRowCount(stationNames.size());

  for (int row = 0; row < stationNames.size(); ++row) {
    m_reservationTable->setItem(row, 0, makeTableItem(stationNames[row]));

    for (int col = 1; col < m_reservationTable->columnCount(); ++col) {
      m_reservationTable->setItem(row, col, makeTableItem(""));
    }
  }
}

void TomasuloWidget::setupRegisterResultTables() {
  m_registerResultTable1 = new QTableWidget(this);
  m_registerResultTable2 = new QTableWidget(this);

  configureCompactRegisterTable(m_registerResultTable1);
  configureCompactRegisterTable(m_registerResultTable2);

  const QStringList registers1 = {"Campo", "F0", "F2", "F4", "F6",
                                  "F8",    "F10", "F12", "F14"};
  const QStringList registers2 = {"Campo", "F16", "F18", "F20", "F22",
                                  "F24",   "F26", "F28", "F30"};

  m_registerResultTable1->setColumnCount(registers1.size());
  m_registerResultTable2->setColumnCount(registers2.size());

  m_registerResultTable1->setHorizontalHeaderLabels(registers1);
  m_registerResultTable2->setHorizontalHeaderLabels(registers2);

  m_registerResultTable1->setRowCount(1);
  m_registerResultTable2->setRowCount(1);

  m_registerResultTable1->setItem(0, 0, makeTableItem("Qi"));
  m_registerResultTable2->setItem(0, 0, makeTableItem("Qi"));

  for (int col = 1; col < m_registerResultTable1->columnCount(); ++col) {
    m_registerResultTable1->setItem(0, col, makeTableItem(""));
  }

  for (int col = 1; col < m_registerResultTable2->columnCount(); ++col) {
    m_registerResultTable2->setItem(0, col, makeTableItem(""));
  }
}

void TomasuloWidget::configureTable(QTableWidget *table, int rowHeight) {
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setSelectionBehavior(QAbstractItemView::SelectItems);
  table->setFocusPolicy(Qt::NoFocus);
  table->setAlternatingRowColors(true);
  table->setMouseTracking(false);

  table->verticalHeader()->setVisible(false);
  table->verticalHeader()->setDefaultSectionSize(rowHeight);

  table->horizontalHeader()->setStretchLastSection(false);
  table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QFont font = table->font();
  font.setPointSize(9);
  table->setFont(font);

  table->setStyleSheet(
      "QTableWidget {"
      "  gridline-color: #d0d0d0;"
      "  alternate-background-color: #f3f3f3;"
      "  selection-background-color: #e6e6e6;"
      "  selection-color: black;"
      "}"
      "QTableWidget::item {"
      "  color: black;"
      "}"
      "QTableWidget::item:hover {"
      "  background-color: #e6e6e6;"
      "  color: black;"
      "}"
      "QTableWidget::item:selected {"
      "  background-color: #e6e6e6;"
      "  color: black;"
      "}"
      "QTableWidget::item:focus {"
      "  outline: none;"
      "}"
      "QTableView::item:hover {"
      "  background-color: #e6e6e6;"
      "  color: black;"
      "}"
      "QTableView::item:selected {"
      "  background-color: #e6e6e6;"
      "  color: black;"
      "}");
}

void TomasuloWidget::configureCompactRegisterTable(QTableWidget *table) {
  configureTable(table, 20);

  table->horizontalHeader()->setFixedHeight(22);
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  table->verticalHeader()->setDefaultSectionSize(20);

  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  table->setFixedHeight(48);
}

void TomasuloWidget::loadProgramText(const QString &programText) {
  const QStringList lines = programText.split('\n');

  QStringList instructions;

  for (QString line : lines) {
    QString trimmed = line.trimmed();

    if (trimmed.isEmpty()) {
      continue;
    }

    if (trimmed.startsWith("#") || trimmed.startsWith("//")) {
      continue;
    }

    const int hashComment = trimmed.indexOf('#');
    if (hashComment >= 0) {
      trimmed = trimmed.left(hashComment).trimmed();
    }

    const int slashComment = trimmed.indexOf("//");
    if (slashComment >= 0) {
      trimmed = trimmed.left(slashComment).trimmed();
    }

    if (trimmed.isEmpty()) {
      continue;
    }

    instructions.append(trimmed);
  }

  m_instructionTable->clearContents();
  m_instructionTable->setRowCount(instructions.size());

  for (int row = 0; row < instructions.size(); ++row) {
    m_instructionTable->setItem(row, 0, makeTableItem(instructions[row]));
    m_instructionTable->setItem(row, 1, makeTableItem(""));
    m_instructionTable->setItem(row, 2, makeTableItem(""));
    m_instructionTable->setItem(row, 3, makeTableItem(""));
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