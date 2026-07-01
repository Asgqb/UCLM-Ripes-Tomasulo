#include "tomasulo_engine.h"

#include <QStringList>

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace Ripes::TomasuloSim {

static QString toQString(const std::string &text) {
  return QString::fromStdString(text);
}

static QString optionalCycleText(const std::optional<std::uint64_t> &cycle) {
  if (!cycle) {
    return "";
  }

  return QString::number(*cycle);
}

static QString operandText(const Operand &operand) {
  const std::string text = operand.toString();
  if (text.empty()) {
    return "";
  }

  return QString::fromStdString(text);
}

static QString stageText(const Stage &stage) {
  return QString::fromStdString(stage.toString());
}

static QStringList cleanProgramLines(const QString &programText) {
  QStringList result;

  const QStringList lines = programText.split('\n');

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

    result.append(trimmed);
  }

  return result;
}

TomasuloEngine::TomasuloEngine() : m_config(defaultConfig()) {
  reset();
}

Config TomasuloEngine::defaultConfig() {
  Config config;

  config.eff_addr_buffer_entries = 2;
  config.fp_add_buffer_entries = 3;
  config.fp_mul_buffer_entries = 3;
  config.int_buffer_entries = 2;
  config.reorder_buffer_entries = 5;

  config.fp_add_buffer_latency = 2;
  config.fp_sub_buffer_latency = 2;
  config.fp_mul_buffer_latency = 5;
  config.fp_div_buffer_latency = 10;

  return config;
}

void TomasuloEngine::loadProgramText(const QString &programText,
                                     const Config &config) {
  m_config = config;
  m_instructions.clear();
  m_rows.clear();
  m_error.clear();

  const QStringList lines = cleanProgramLines(programText);

  for (const QString &line : lines) {
    Row row;
    row.instruction = line;
    m_rows.push_back(row);

    try {
      m_instructions.push_back(RiscVOp::parse(line.toStdString()));
    } catch (const std::exception &error) {
      m_error = "Parse error in instruction '" + line + "': " + error.what();
      m_instructions.clear();
      break;
    }
  }

  reset();
}

void TomasuloEngine::reset() {
  m_reorderBuffer = std::make_unique<ReorderBuffer>(m_config);
  m_nextInstruction = 0;
  m_nextCycle = 1;
  m_visibleCycle = 0;
  m_finished = false;

  for (Row &row : m_rows) {
    row.issued.reset();
    row.startEx.reset();
    row.endEx.reset();
    row.memAccess.reset();
    row.writeBack.reset();
    row.committed.reset();
  }

  if (!m_error.isEmpty() || m_instructions.empty()) {
    m_finished = true;
  }
}

void TomasuloEngine::ensureRow(std::size_t index) {
  while (m_rows.size() <= index) {
    m_rows.push_back(Row{});
  }
}

void TomasuloEngine::recordStage(std::size_t instructionNum, const RiscVOp &op,
                                 const Stage &stage, std::uint64_t cycle) {
  ensureRow(instructionNum);

  if (m_rows[instructionNum].instruction.isEmpty()) {
    m_rows[instructionNum].instruction = toQString(op.toString());
  }

  switch (stage.kind) {
  case StageKind::Execute:
    if (stage.cyclesLeft == 1 && !m_rows[instructionNum].startEx) {
      m_rows[instructionNum].startEx = cycle;
      m_rows[instructionNum].endEx = cycle;
    } else if (stage.cyclesLeft == 1 && m_rows[instructionNum].startEx) {
      m_rows[instructionNum].endEx = cycle;
    } else if (!m_rows[instructionNum].startEx) {
      m_rows[instructionNum].startEx = cycle;
    }
    break;

  case StageKind::MemAccess:
    m_rows[instructionNum].memAccess = cycle;
    break;

  case StageKind::WriteBack:
    m_rows[instructionNum].writeBack = cycle;
    break;

  case StageKind::Commit:
    m_rows[instructionNum].committed = cycle;
    break;

  case StageKind::Issue:
  case StageKind::WaitingToCommit:
    break;
  }
}

void TomasuloEngine::step() {
  if (m_finished || !m_reorderBuffer) {
    return;
  }

  if (m_reorderBuffer->getFinishedInstructions() >= m_instructions.size()) {
    m_finished = true;
    return;
  }

  const std::uint64_t currentCycle = m_nextCycle;

  if (m_nextInstruction < m_instructions.size()) {
    const RiscVOp op = m_instructions[m_nextInstruction];

    if (m_reorderBuffer->add(op)) {
      ensureRow(m_nextInstruction);
      m_rows[m_nextInstruction].issued = currentCycle;
      m_nextInstruction++;
    }
  }

  const auto stagesBeforeTick = m_reorderBuffer->getStages();
  for (const auto &[instructionNum, op, stage] : stagesBeforeTick) {
    recordStage(instructionNum, op, stage, currentCycle);
  }

  m_reorderBuffer->tick(currentCycle);

  m_nextCycle++;

  const auto stagesAfterTick = m_reorderBuffer->getStages();
  for (const auto &[instructionNum, op, stage] : stagesAfterTick) {
    recordStage(instructionNum, op, stage, m_nextCycle);
  }

  m_visibleCycle = currentCycle;

  if (m_nextInstruction >= m_instructions.size() &&
      m_reorderBuffer->getFinishedInstructions() >= m_instructions.size()) {
    m_finished = true;
  }
}

void TomasuloEngine::runToCompletion(std::uint64_t maxCycles) {
  std::uint64_t guard = 0;

  while (!isFinished() && guard < maxCycles) {
    step();
    guard++;
  }

  if (guard >= maxCycles) {
    m_error = "Execution stopped: maximum cycle guard reached.";
    m_finished = true;
  }
}

bool TomasuloEngine::isFinished() const {
  return m_finished;
}

QString TomasuloEngine::executionRange(const Row &row) const {
  if (!row.startEx) {
    return "";
  }

  if (!row.endEx) {
    return QString::number(*row.startEx) + "-";
  }

  if (*row.startEx == *row.endEx) {
    return QString::number(*row.startEx);
  }

  return QString::number(*row.startEx) + "-" + QString::number(*row.endEx);
}

std::vector<TomasuloReservationStationStatus>
TomasuloEngine::buildReservationStations() const {
  std::vector<TomasuloReservationStationStatus> result;

  if (!m_reorderBuffer) {
    return result;
  }

  const auto stages = m_reorderBuffer->getStages();

  int addStation = 0;
  int multStation = 0;
  int loadStation = 0;
  int storeStation = 0;
  int intStation = 0;
  int genericStation = 0;

  for (const auto &[instructionNum, op, stage] : stages) {
    TomasuloReservationStationStatus station;

    if (op.isFpAdd()) {
      station.name = "Add" + QString::number(addStation++);
    } else if (op.isFpMul()) {
      station.name = "Mult" + QString::number(multStation++);
    } else if (op.isLoad()) {
      station.name = "Load" + QString::number(loadStation++);
    } else if (op.isStore()) {
      station.name = "Store" + QString::number(storeStation++);
    } else if (op.isAlu() || op.isBranch()) {
      station.name = "Int" + QString::number(intStation++);
    } else {
      station.name = "RS" + QString::number(genericStation++);
    }

    station.busy = "Yes";

    const QString fullInstruction = toQString(op.toString());
    const int firstSpace = fullInstruction.indexOf(' ');
    if (firstSpace >= 0) {
      station.operation = fullInstruction.left(firstSpace);
    } else {
      station.operation = fullInstruction;
    }

    station.vj = operandText(op.src1());
    station.vk = operandText(op.src2());

    if (stage.kind == StageKind::Issue) {
      station.qj = "waiting";
      station.qk = "waiting";
    } else {
      station.qj = "";
      station.qk = "";
    }

    if (auto address = op.addr()) {
      station.address = QString::number(*address);
    } else {
      station.address = stageText(stage);
    }

    result.push_back(station);
  }

  return result;
}
std::vector<TomasuloRegisterResultStatus> TomasuloEngine::buildRegisters()
    const {
  std::vector<TomasuloRegisterResultStatus> result;

  for (int reg = 0; reg <= 30; reg += 2) {
    TomasuloRegisterResultStatus status;
    status.registerName = "F" + QString::number(reg);
    status.qi = "";
    result.push_back(status);
  }

  if (!m_reorderBuffer) {
    return result;
  }

  std::unordered_map<int, QString> qiByRegister;

  const auto stages = m_reorderBuffer->getStages();

  for (const auto &[instructionNum, op, stage] : stages) {
    if (stage.kind == StageKind::WaitingToCommit ||
        stage.kind == StageKind::Commit) {
      continue;
    }

    if (stage.kind == StageKind::WriteBack) {
      continue;
    }

    const auto dst = op.dst();
    if (!dst) {
      continue;
    }

    try {
      const Register reg = dst->asReg();
      if (reg.kind == RegisterKind::FP && reg.number <= 30 &&
          reg.number % 2 == 0) {
        qiByRegister[static_cast<int>(reg.number)] =
            "I" + QString::number(instructionNum);
      }
    } catch (...) {
    }
  }

  for (TomasuloRegisterResultStatus &status : result) {
    bool ok = false;
    const int regNumber = status.registerName.mid(1).toInt(&ok);

    if (!ok) {
      continue;
    }

    const auto it = qiByRegister.find(regNumber);
    if (it != qiByRegister.end()) {
      status.qi = it->second;
    }
  }

  return result;
}

TomasuloSnapshot TomasuloEngine::snapshot() const {
  TomasuloSnapshot snapshot;
  snapshot.cycle = m_visibleCycle;
  snapshot.instructionsRetired = 0;
  for (const Row &row : m_rows) {
    if (row.committed) {
      snapshot.instructionsRetired++;
    }
  }
  snapshot.finished = m_finished;
  snapshot.error = m_error;

  for (const Row &row : m_rows) {
    TomasuloInstructionStatus status;
    status.instruction = row.instruction;
    status.issueCycle = optionalCycleText(row.issued);
    status.executionCycles = executionRange(row);
    status.writeBackCycle = optionalCycleText(row.writeBack);
    snapshot.instructions.push_back(status);
  }

  snapshot.reservationStations = buildReservationStations();
  snapshot.registers = buildRegisters();

  return snapshot;
}

} // namespace Ripes::TomasuloSim



