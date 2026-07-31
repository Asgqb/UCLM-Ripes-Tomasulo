#pragma once

#include "tomasulo_engine.h"

#include <QString>
#include <QWidget>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class QTableWidget;

namespace Ripes {

class TomasuloWidget : public QWidget {
public:
  explicit TomasuloWidget(QWidget *parent = nullptr);
  ~TomasuloWidget() override;

  void loadProgramText(const QString &programText);

  void resetSimulation(const QString &programText,
                       const TomasuloSim::Config &config);

  void clock();
  void stepBack();
  void stepForward();
  void runToCompletion();

  bool canStepBack() const;
  bool canStepForward() const;

  bool isFinished() const;
  std::uint64_t currentCycle() const;
  std::uint64_t instructionsRetired() const;

private:
  void setupUi();
  void setupInstructionTable();
  void setupReservationStationsTable();
  void setupRegisterResultTables();

  void configureTable(QTableWidget *table, int rowHeight = 22);
  void configureCompactRegisterTable(QTableWidget *table);

  void loadSnapshot(const TomasuloSim::TomasuloSnapshot &snapshot);
  void clearRegisterTables();

  void rebuildEngineToHistoryIndex();
  const TomasuloSim::TomasuloSnapshot *currentHistorySnapshot() const;

  QTableWidget *m_instructionTable = nullptr;
  QTableWidget *m_reservationTable = nullptr;
  QTableWidget *m_registerResultTable1 = nullptr;
  QTableWidget *m_registerResultTable2 = nullptr;

  std::unique_ptr<TomasuloSim::TomasuloEngine> m_engine;

  QString m_programText;
  TomasuloSim::Config m_config;
  std::vector<TomasuloSim::TomasuloSnapshot> m_history;
  std::size_t m_historyIndex = 0;
};

} // namespace Ripes
