#include "tomasulo.h"

#include <QAbstractItemView>
#include <QFont>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace Ripes {

TomasuloWidget::TomasuloWidget(QWidget *parent) : QWidget(parent) {
  setupUi();
  fillDummyData();
}

void TomasuloWidget::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(6, 6, 6, 6);
  mainLayout->setSpacing(6);

  QFont compactFont = font();
  compactFont.setPointSize(9);
  setFont(compactFont);

  auto *topLayout = new QHBoxLayout();
  topLayout->setContentsMargins(0, 0, 0, 0);
  topLayout->setSpacing(6);

  auto *instructionBox = new QGroupBox("Estado instruccion", this);
  auto *instructionLayout = new QVBoxLayout(instructionBox);
  instructionLayout->setContentsMargins(4, 4, 4, 4);
  instructionLayout->setSpacing(4);

  setupInstructionTable();
  instructionLayout->addWidget(m_instructionTable);

  auto *reservationBox =
      new QGroupBox("Estado de las estaciones de reserva", this);
  auto *reservationLayout = new QVBoxLayout(reservationBox);
  reservationLayout->setContentsMargins(4, 4, 4, 4);
  reservationLayout->setSpacing(4);

  setupReservationStationsTable();
  reservationLayout->addWidget(m_reservationTable);

  instructionBox->setMinimumHeight(250);
  reservationBox->setMinimumHeight(250);

  topLayout->addWidget(instructionBox, 1);
  topLayout->addWidget(reservationBox, 2);

  auto *registerBox =
      new QGroupBox("Estado de los registros resultado", this);
  registerBox->setMaximumHeight(132);

  auto *registerLayout = new QVBoxLayout(registerBox);
  registerLayout->setContentsMargins(2, 2, 2, 2);
  registerLayout->setSpacing(2);

  setupRegisterResultTables();
  registerLayout->addWidget(m_registerResultTable1);
  registerLayout->addWidget(m_registerResultTable2);

  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(registerBox);
}

void TomasuloWidget::configureTable(QTableWidget *table, int rowHeight) {
  table->verticalHeader()->setVisible(false);
  table->verticalHeader()->setDefaultSectionSize(rowHeight);

  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setAlternatingRowColors(false);
  table->setShowGrid(true);
  table->setFont(font());

  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void TomasuloWidget::configureCompactRegisterTable(QTableWidget *table) {
  configureTable(table, 20);

  table->horizontalHeader()->setFixedHeight(22);
  table->verticalHeader()->setDefaultSectionSize(20);

  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  table->setFixedHeight(48);
}

void TomasuloWidget::setupInstructionTable() {
  m_instructionTable = new QTableWidget(this);

  m_instructionTable->setColumnCount(4);
  m_instructionTable->setHorizontalHeaderLabels(
      {"Instruccion", "Emision", "Ejecucion", "Escritura"});

  configureTable(m_instructionTable, 22);

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

  m_instructionTable->setMinimumHeight(210);
}

void TomasuloWidget::setupReservationStationsTable() {
  m_reservationTable = new QTableWidget(this);

  m_reservationTable->setColumnCount(8);
  m_reservationTable->setHorizontalHeaderLabels(
      {"Nombre", "Ocupada", "Operacion", "Vj", "Vk", "Qj", "Qk", "A"});

  configureTable(m_reservationTable, 22);
  m_reservationTable->setMinimumHeight(210);
}

void TomasuloWidget::setupRegisterResultTables() {
  m_registerResultTable1 = new QTableWidget(this);
  m_registerResultTable1->setColumnCount(9);
  m_registerResultTable1->setRowCount(1);
  m_registerResultTable1->setHorizontalHeaderLabels(
      {"Campo", "F0", "F2", "F4", "F6", "F8", "F10", "F12", "F14"});

  configureCompactRegisterTable(m_registerResultTable1);

  m_registerResultTable2 = new QTableWidget(this);
  m_registerResultTable2->setColumnCount(9);
  m_registerResultTable2->setRowCount(1);
  m_registerResultTable2->setHorizontalHeaderLabels(
      {"Campo", "F16", "F18", "F20", "F22", "F24", "F26", "F28", "F30"});

  configureCompactRegisterTable(m_registerResultTable2);
}

void TomasuloWidget::fillDummyData() {
  m_instructionTable->setRowCount(12);

  m_instructionTable->setItem(0, 0,
                              new QTableWidgetItem("lwc1 $f0, 0($t0)"));
  m_instructionTable->setItem(0, 1, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(0, 2, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(0, 3, new QTableWidgetItem("✓"));

  m_instructionTable->setItem(1, 0,
                              new QTableWidgetItem("lwc1 $f2, 4($t0)"));
  m_instructionTable->setItem(1, 1, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(1, 2, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(1, 3, new QTableWidgetItem("✓"));

  m_instructionTable->setItem(2, 0,
                              new QTableWidgetItem("lwc1 $f4, 8($t0)"));
  m_instructionTable->setItem(2, 1, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(2, 2, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(2, 3, new QTableWidgetItem(""));

  m_instructionTable->setItem(3, 0,
                              new QTableWidgetItem("lwc1 $f6, 12($t0)"));
  m_instructionTable->setItem(3, 1, new QTableWidgetItem("✓"));
  m_instructionTable->setItem(3, 2, new QTableWidgetItem(""));
  m_instructionTable->setItem(3, 3, new QTableWidgetItem(""));

  m_instructionTable->setItem(4, 0,
                              new QTableWidgetItem("add.s $f8, $f0, $f2"));
  m_instructionTable->setItem(5, 0,
                              new QTableWidgetItem("mul.s $f10, $f4, $f6"));
  m_instructionTable->setItem(6, 0,
                              new QTableWidgetItem("sub.s $f12, $f8, $f2"));
  m_instructionTable->setItem(7, 0,
                              new QTableWidgetItem("div.s $f14, $f10, $f6"));
  m_instructionTable->setItem(8, 0,
                              new QTableWidgetItem("add.s $f16, $f12, $f14"));
  m_instructionTable->setItem(9, 0,
                              new QTableWidgetItem("mul.s $f18, $f16, $f2"));
  m_instructionTable->setItem(10, 0,
                              new QTableWidgetItem("swc1 $f18, 16($t0)"));
  m_instructionTable->setItem(11, 0,
                              new QTableWidgetItem("lwc1 $f20, 20($t0)"));

  for (int row = 0; row < m_instructionTable->rowCount(); ++row) {
    for (int col = 1; col < m_instructionTable->columnCount(); ++col) {
      if (!m_instructionTable->item(row, col)) {
        m_instructionTable->setItem(row, col, new QTableWidgetItem(""));
      }
    }
  }

  m_reservationTable->setRowCount(12);

  m_reservationTable->setItem(0, 0, new QTableWidgetItem("Add0"));
  m_reservationTable->setItem(1, 0, new QTableWidgetItem("Add1"));
  m_reservationTable->setItem(2, 0, new QTableWidgetItem("Add2"));
  m_reservationTable->setItem(3, 0, new QTableWidgetItem("Mult0"));
  m_reservationTable->setItem(4, 0, new QTableWidgetItem("Mult1"));
  m_reservationTable->setItem(5, 0, new QTableWidgetItem("Divide0"));
  m_reservationTable->setItem(6, 0, new QTableWidgetItem("Load0"));
  m_reservationTable->setItem(7, 0, new QTableWidgetItem("Load1"));
  m_reservationTable->setItem(8, 0, new QTableWidgetItem("Load2"));
  m_reservationTable->setItem(9, 0, new QTableWidgetItem("Store0"));
  m_reservationTable->setItem(10, 0, new QTableWidgetItem("Store1"));
  m_reservationTable->setItem(11, 0, new QTableWidgetItem("Store2"));

  for (int row = 0; row < m_reservationTable->rowCount(); ++row) {
    for (int col = 1; col < m_reservationTable->columnCount(); ++col) {
      m_reservationTable->setItem(row, col, new QTableWidgetItem(""));
    }
  }

  m_reservationTable->setItem(0, 1, new QTableWidgetItem("✓"));
  
  m_reservationTable->setItem(6, 1, new QTableWidgetItem("✓"));
  m_reservationTable->setItem(6, 2, new QTableWidgetItem("Carga"));
  m_reservationTable->setItem(6, 3, new QTableWidgetItem("0x10010000"));
  m_reservationTable->setItem(6, 7, new QTableWidgetItem("0x40400000"));

  m_reservationTable->setItem(7, 1, new QTableWidgetItem("✓"));
  m_reservationTable->setItem(7, 2, new QTableWidgetItem("Carga"));
  m_reservationTable->setItem(7, 3, new QTableWidgetItem("0x10010004"));
  m_reservationTable->setItem(7, 7, new QTableWidgetItem("0x40800000"));

  m_registerResultTable1->setItem(0, 0, new QTableWidgetItem("Qi"));
  for (int col = 1; col < m_registerResultTable1->columnCount(); ++col) {
    m_registerResultTable1->setItem(0, col, new QTableWidgetItem(""));
  }

  m_registerResultTable1->setItem(0, 3, new QTableWidgetItem("Load0"));
  m_registerResultTable1->setItem(0, 4, new QTableWidgetItem("Load1"));
  m_registerResultTable1->setItem(0, 5, new QTableWidgetItem("Add0"));

  m_registerResultTable2->setItem(0, 0, new QTableWidgetItem("Qi"));
  for (int col = 1; col < m_registerResultTable2->columnCount(); ++col) {
    m_registerResultTable2->setItem(0, col, new QTableWidgetItem(""));
  }
}

} // namespace Ripes