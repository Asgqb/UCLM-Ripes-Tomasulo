#include "tomasulo_pipeline.h"

#include <iostream>
#include <stdexcept>

namespace Ripes::TomasuloSim {

Stage Stage::issue() {
    return Stage{StageKind::Issue, 0};
}

Stage Stage::execute(std::uint64_t cycles) {
    return Stage{StageKind::Execute, cycles};
}

Stage Stage::memAccess() {
    return Stage{StageKind::MemAccess, 0};
}

Stage Stage::writeBack() {
    return Stage{StageKind::WriteBack, 0};
}

Stage Stage::waitingToCommit() {
    return Stage{StageKind::WaitingToCommit, 0};
}

Stage Stage::commit() {
    return Stage{StageKind::Commit, 0};
}

bool Stage::sameKind(StageKind otherKind) const {
    return kind == otherKind;
}

bool Stage::equals(const Stage& other) const {
    return kind == other.kind && cyclesLeft == other.cyclesLeft;
}

std::string Stage::toString() const {
    switch (kind) {
        case StageKind::Issue:
            return "Issue";

        case StageKind::Execute:
            return "Execute(" + std::to_string(cyclesLeft) + ")";

        case StageKind::MemAccess:
            return "MemAccess";

        case StageKind::WriteBack:
            return "WriteBack";

        case StageKind::WaitingToCommit:
            return "WaitingToCommit";

        case StageKind::Commit:
            return "Commit";
    }

    return "Unknown";
}

std::ostream& operator<<(std::ostream& os, const Stage& stage) {
    os << stage.toString();
    return os;
}

ReorderBuffer::ReorderBuffer(const Config& config)
    : config(config) {
    size = static_cast<std::size_t>(config.reorder_buffer_entries);

    entries.clear();
    entries.resize(size);

    availableReservationStations[FunctionalUnit::ALU] =
        static_cast<std::size_t>(config.int_buffer_entries);

    availableReservationStations[FunctionalUnit::FPUAdd] =
        static_cast<std::size_t>(config.fp_add_buffer_entries);

    availableReservationStations[FunctionalUnit::FPUMul] =
        static_cast<std::size_t>(config.fp_mul_buffer_entries);

    availableReservationStations[FunctionalUnit::EffectAddr] =
        static_cast<std::size_t>(config.eff_addr_buffer_entries);
}

std::uint64_t ReorderBuffer::latencyFor(const RiscVOp& op) const {
    if (op.isFpDiv()) {
        return config.fp_div_buffer_latency;
    }

    if (op.isFpMul()) {
        return config.fp_mul_buffer_latency;
    }

    if (op.isFpAdd()) {
        return config.fp_add_buffer_latency;
    }

    if (op.isBranch()) {
        return 1;
    }

    if (op.isAlu()) {
        return 1;
    }

    return 1;
}

bool ReorderBuffer::add(const RiscVOp& op) {
    if (entriesUsed >= size) {
        reorderBufferDelays++;
        return false;
    }

    FunctionalUnit unit = op.functionalUnit();

    auto stationIt = availableReservationStations.find(unit);

    if (stationIt == availableReservationStations.end()) {
        reservationStationDelays++;
        return false;
    }

    if (stationIt->second == 0) {
        reservationStationDelays++;
        return false;
    }

    if (auto address = op.addr()) {
        if (addressesLoaded.find(*address) != addressesLoaded.end()) {
            dataMemoryConflictDelays++;
            return false;
        }

        if (addressesStored.find(*address) != addressesStored.end()) {
            dataMemoryConflictDelays++;
            return false;
        }
    }

    availableReservationStations[unit]--;

    if (auto dst = op.dst()) {
        Register dstReg = dst->asReg();
        registerMapping[dstReg.toString()] = static_cast<std::uint64_t>(head);
    }

    entries[head] = ReorderBufferEntry{
        issueCount,
        op,
        Stage::issue()
    };

    head = (head + 1) % size;
    entriesUsed++;
    issueCount++;

    return true;
}

std::vector<std::tuple<std::size_t, RiscVOp, Stage>> ReorderBuffer::getStages() const {
    std::vector<std::tuple<std::size_t, RiscVOp, Stage>> result;

    for (std::size_t i = tail; i < tail + entriesUsed; ++i) {
        std::size_t index = i % size;

        if (entries[index]) {
            result.push_back({
                entries[index]->issuedIndex,
                entries[index]->op,
                entries[index]->stage
            });
        }
    }

    return result;
}

std::vector<std::pair<std::size_t, RiscVOp>> ReorderBuffer::getAllInStage(const Stage& stage) const {
    std::vector<std::pair<std::size_t, RiscVOp>> result;

    for (std::size_t i = head; i < head + size; ++i) {
        std::size_t index = i % size;

        if (entries[index] && entries[index]->stage.equals(stage)) {
            result.push_back({index, entries[index]->op});
        }
    }

    return result;
}

std::vector<std::pair<std::size_t, RiscVOp>> ReorderBuffer::getAllInEx() const {
    std::vector<std::pair<std::size_t, RiscVOp>> result;

    for (std::size_t i = head; i < head + size; ++i) {
        std::size_t index = i % size;

        if (entries[index] && entries[index]->stage.kind == StageKind::Execute) {
            result.push_back({index, entries[index]->op});
        }
    }

    return result;
}

bool ReorderBuffer::allPreviousCommitted(std::size_t index) const {
    for (std::size_t j = tail; j < tail + size; ++j) {
        std::size_t current = j % size;

        if (current == index) {
            break;
        }

        if (entries[current] && entries[current]->stage.kind != StageKind::Commit) {
            return false;
        }
    }

    return true;
}

bool ReorderBuffer::writeToCdb(std::size_t index) {
    if (index >= entries.size()) {
        return false;
    }

    if (!entries[index]) {
        return false;
    }

    if (entries[index]->stage.kind != StageKind::WriteBack) {
        return false;
    }

    if (allPreviousCommitted(index)) {
        entries[index]->stage = Stage::commit();
    } else {
        entries[index]->stage = Stage::waitingToCommit();
    }

    return true;
}

bool ReorderBuffer::isEarlierThan(std::size_t i, std::size_t j) const {
    if (i == j) {
        return false;
    }

    if (i < tail && j < tail) {
        return i < j;
    }

    if (i >= tail && j >= tail) {
        return i < j;
    }

    if (i < tail && j >= tail) {
        return true;
    }

    if (i >= tail && j < tail) {
        return false;
    }

    return false;
}

void ReorderBuffer::freeReservationStation(const RiscVOp& op) {
    availableReservationStations[op.functionalUnit()]++;
}

void ReorderBuffer::removeRegisterMappingForDestination(const RiscVOp& op) {
    if (auto dst = op.dst()) {
        Register dstReg = dst->asReg();
        registerMapping.erase(dstReg.toString());
    }
}

void ReorderBuffer::tick() {
    tick(0);
}

void ReorderBuffer::tick(std::uint64_t /*cycle*/) {
    bool alreadyCommitted = false;

    // 1. Commit stage: remove committed instructions from ROB.
    for (const auto& [index, op] : getAllInStage(Stage::commit())) {
        if (auto address = op.addr()) {
            if (op.isLoad()) {
                addressesLoaded.erase(*address);
            } else {
                addressesStored.erase(*address);
            }
        }

        entriesCommitted++;
        entries[index] = std::nullopt;
        tail = (tail + 1) % size;
        entriesUsed--;
    }

    // 2. WaitingToCommit stage: only one instruction may commit per cycle.
    for (const auto& [index, op] : getAllInStage(Stage::waitingToCommit())) {
        (void)op;

        if (allPreviousCommitted(index) && !alreadyCommitted) {
            entries[index]->stage = Stage::commit();
            alreadyCommitted = true;
        }
    }

    // 3. WriteBack stage: only one instruction writes to the CDB per cycle.
    bool wroteToCdb = false;

    for (const auto& [index, op] : getAllInStage(Stage::writeBack())) {
        if (wroteToCdb) {
            continue;
        }

        if (allPreviousCommitted(index) && !alreadyCommitted) {
            entries[index]->stage = Stage::commit();
            alreadyCommitted = true;
        } else {
            entries[index]->stage = Stage::waitingToCommit();
        }

        removeRegisterMappingForDestination(op);
        wroteToCdb = true;
    }

    // 4. MemAccess stage: only one memory access per cycle.
    bool alreadyAccessedMemory = false;

    for (const auto& [index, op] : getAllInStage(Stage::memAccess())) {
        if (auto address = op.addr()) {
            if (addressesStored.find(*address) != addressesStored.end()) {
                dataMemoryConflictDelays++;
                continue;
            }

            if (op.isLoad()) {
                addressesLoaded.insert(*address);
            } else {
                addressesStored.insert(*address);
            }
        }

        if (!alreadyAccessedMemory) {
            entries[index]->stage = Stage::writeBack();
            alreadyAccessedMemory = true;
        }
    }

    // Extra CDB pass after memory, matching the structure of the Rust version.
    for (const auto& [index, op] : getAllInStage(Stage::writeBack())) {
        (void)index;

        if (wroteToCdb) {
            continue;
        }

        if (!op.addr()) {
            removeRegisterMappingForDestination(op);
        }
    }

    // 5. Execute stage: decrement execution cycles and move finished instructions.
    for (const auto& [index, op] : getAllInEx()) {
        if (!entries[index]) {
            continue;
        }

        Stage& stage = entries[index]->stage;

        if (stage.kind != StageKind::Execute) {
            continue;
        }

        if (stage.cyclesLeft > 0) {
            stage.cyclesLeft--;
        }

        if (stage.cyclesLeft == 0) {
            if (op.accessesMemory()) {
                entries[index]->stage = Stage::memAccess();
                freeReservationStation(op);
            } else if (op.writesBack()) {
                if (!wroteToCdb) {
                    entries[index]->stage = Stage::writeBack();
                    freeReservationStation(op);
                }
            } else {
                if (allPreviousCommitted(index) && !alreadyCommitted) {
                    entries[index]->stage = Stage::commit();
                    alreadyCommitted = true;
                } else {
                    entries[index]->stage = Stage::waitingToCommit();
                }

                freeReservationStation(op);
            }
        }
    }

    // 6. Issue stage: wait for RAW dependencies, then start execution.
    for (const auto& [index, op] : getAllInStage(Stage::issue())) {
        bool blockedByDependency = false;

        if (auto src1 = op.src1().depReg()) {
            if (auto dst = op.dst()) {
                if (src1->toString() != dst->asReg().toString() &&
                    registerMapping.find(src1->toString()) != registerMapping.end()) {
                    trueDependenceDelays++;
                    blockedByDependency = true;
                }
            }
        }

        if (!blockedByDependency) {
            if (auto src2 = op.src2().depReg()) {
                if (auto dst = op.dst()) {
                    if (src2->toString() != dst->asReg().toString() &&
                        registerMapping.find(src2->toString()) != registerMapping.end()) {
                        trueDependenceDelays++;
                        blockedByDependency = true;
                    }
                }
            }
        }

        if (blockedByDependency) {
            continue;
        }

        entries[index]->stage = Stage::execute(latencyFor(op));
    }

    // Final CDB clean-up pass, matching the original Rust structure.
    for (const auto& [index, op] : getAllInStage(Stage::writeBack())) {
        (void)index;

        if (wroteToCdb) {
            continue;
        }

        removeRegisterMappingForDestination(op);
    }
}

std::size_t ReorderBuffer::usedEntries() const {
    return entriesUsed;
}

std::size_t ReorderBuffer::totalEntries() const {
    return size;
}

std::size_t ReorderBuffer::committedEntries() const {
    return entriesCommitted;
}

std::size_t ReorderBuffer::getFinishedInstructions() const {
    return entriesCommitted;
}

std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t> ReorderBuffer::getDelays() const {
    return {
        reorderBufferDelays,
        reservationStationDelays,
        dataMemoryConflictDelays,
        trueDependenceDelays
    };
}

void ReorderBuffer::printDebug(std::ostream& os) const {
    os << "Reorder Buffer:\n";

    os << " Register mapping:\n";
    for (const auto& [reg, index] : registerMapping) {
        os << "  " << reg << " -> " << index << "\n";
    }

    os << " Available Reservation stations:\n";
    for (const auto& [unit, available] : availableReservationStations) {
        os << "  " << functionalUnitToString(unit) << " -> " << available << "\n";
    }

    os << " Addresses stored:\n";
    for (const auto& address : addressesStored) {
        os << "  " << address << "\n";
    }

    os << " Addresses loaded:\n";
    for (const auto& address : addressesLoaded) {
        os << "  " << address << "\n";
    }

    os << " Head: " << head << "\n";
    os << " Tail: " << tail << "\n";
    os << " Entries used: " << entriesUsed << "\n";
    os << " Entries committed: " << entriesCommitted << "\n";

    os << " Entries:\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        os << "  #" << i << ") ";

        if (entries[i]) {
            os << entries[i]->op
               << " (" << entries[i]->stage << ")"
               << " issued on " << entries[i]->issuedIndex
               << "\n";
        } else {
            os << "None\n";
        }
    }
}

} // namespace Ripes::TomasuloSim

