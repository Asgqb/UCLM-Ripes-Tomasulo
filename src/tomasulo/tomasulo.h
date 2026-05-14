#pragma once

#include <QWidget>

class QTableWidget;

namespace Ripes {

class TomasuloWidget : public QWidget {
public:
  explicit TomasuloWidget(QWidget *parent = nullptr);

private:
  void setupUi();
  void setupInstructionTable();
  void setupReservationStationsTable();
  void setupRegisterResultTables();

  void configureTable(QTableWidget *table, int rowHeight = 22);
  void configureCompactRegisterTable(QTableWidget *table);

  void fillDummyData();

  QTableWidget *m_instructionTable = nullptr;
  QTableWidget *m_reservationTable = nullptr;
  QTableWidget *m_registerResultTable1 = nullptr;
  QTableWidget *m_registerResultTable2 = nullptr;
};

} // namespace Ripes