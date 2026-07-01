#pragma once

#include "tomasulo_config.h"
#include "tomasulo_pipeline.h"

#include <QString>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace Ripes::TomasuloSim {

struct TomasuloInstructionStatus {
  QString instruction;
  QString issueCycle;
  QString executionCycles;
  QString writeBackCycle;
};

struct TomasuloReservationStationStatus {
  QString name;
  QString busy;
  QString operation;
  QString vj;
  QString vk;
  QString qj;
  QString qk;
  QString address;
};

struct TomasuloRegisterResultStatus {
  QString registerName;
  QString qi;
};

struct TomasuloSnapshot {
  std::uint64_t cycle = 0;
  std::uint64_t instructionsRetired = 0;
  bool finished = false;
  QString error;

  std::vector<TomasuloInstructionStatus> instructions;
  std::vector<TomasuloReservationStationStatus> reservationStations;
  std::vector<TomasuloRegisterResultStatus> registers;
};

class TomasuloEngine {
public:
  TomasuloEngine();

  static Config defaultConfig();

  void loadProgramText(const QString &programText, const Config &config);
  void reset();
  void step();
  void runToCompletion(std::uint64_t maxCycles = 100000);

  bool isFinished() const;
  TomasuloSnapshot snapshot() const;

private:
  struct Row {
    QString instruction;
    std::optional<std::uint64_t> issued;
    std::optional<std::uint64_t> startEx;
    std::optional<std::uint64_t> endEx;
    std::optional<std::uint64_t> memAccess;
    std::optional<std::uint64_t> writeBack;
    std::optional<std::uint64_t> committed;
  };

  void ensureRow(std::size_t index);
  void recordStage(std::size_t instructionNum, const RiscVOp &op,
                   const Stage &stage, std::uint64_t cycle);

  QString executionRange(const Row &row) const;
  std::vector<TomasuloReservationStationStatus> buildReservationStations()
      const;
  std::vector<TomasuloRegisterResultStatus> buildRegisters() const;

  Config m_config;
  std::vector<RiscVOp> m_instructions;
  std::vector<Row> m_rows;
  std::unique_ptr<ReorderBuffer> m_reorderBuffer;

  std::size_t m_nextInstruction = 0;
  std::uint64_t m_nextCycle = 1;
  std::uint64_t m_visibleCycle = 0;
  bool m_finished = false;
  QString m_error;
};

} // namespace Ripes::TomasuloSim


