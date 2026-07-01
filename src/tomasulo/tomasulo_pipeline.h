#pragma once

#include "tomasulo_config.h"
#include "tomasulo_op.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace Ripes::TomasuloSim {

enum class StageKind {
    Issue,
    Execute,
    MemAccess,
    WriteBack,
    WaitingToCommit,
    Commit
};

struct Stage {
    StageKind kind;
    std::uint64_t cyclesLeft = 0;

    static Stage issue();
    static Stage execute(std::uint64_t cycles);
    static Stage memAccess();
    static Stage writeBack();
    static Stage waitingToCommit();
    static Stage commit();

    bool sameKind(StageKind otherKind) const;
    bool equals(const Stage& other) const;

    std::string toString() const;
};

struct ReorderBufferEntry {
    std::size_t issuedIndex;
    RiscVOp op;
    Stage stage;
};

class ReorderBuffer {
public:
    explicit ReorderBuffer(const Config& config);

    bool add(const RiscVOp& op);

    void tick();
    void tick(std::uint64_t cycle);

    std::uint64_t latencyFor(const RiscVOp& op) const;

    std::vector<std::tuple<std::size_t, RiscVOp, Stage>> getStages() const;

    std::size_t usedEntries() const;
    std::size_t totalEntries() const;
    std::size_t committedEntries() const;
    std::size_t getFinishedInstructions() const;

    std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t> getDelays() const;

    std::vector<std::pair<std::size_t, RiscVOp>> getAllInStage(const Stage& stage) const;
    std::vector<std::pair<std::size_t, RiscVOp>> getAllInEx() const;

    bool writeToCdb(std::size_t index);
    bool isEarlierThan(std::size_t i, std::size_t j) const;

    void printDebug(std::ostream& os) const;

private:
    Config config;

    std::map<std::string, std::uint64_t> registerMapping;

    std::set<std::uint64_t> addressesStored;
    std::set<std::uint64_t> addressesLoaded;

    std::map<FunctionalUnit, std::size_t> availableReservationStations;

    std::vector<std::optional<ReorderBufferEntry>> entries;

    std::size_t issueCount = 0;
    std::size_t head = 0;
    std::size_t tail = 0;
    std::size_t size = 0;
    std::size_t entriesUsed = 0;
    std::size_t entriesCommitted = 0;

    std::uint64_t reorderBufferDelays = 0;
    std::uint64_t reservationStationDelays = 0;
    std::uint64_t dataMemoryConflictDelays = 0;
    std::uint64_t trueDependenceDelays = 0;

private:
    bool allPreviousCommitted(std::size_t index) const;
    void freeReservationStation(const RiscVOp& op);
    void removeRegisterMappingForDestination(const RiscVOp& op);
};

std::ostream& operator<<(std::ostream& os, const Stage& stage);

} // namespace Ripes::TomasuloSim

