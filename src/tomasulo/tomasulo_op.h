#pragma once

#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace Ripes::TomasuloSim {

enum class FunctionalUnit {
    ALU,
    EffectAddr,
    FPUMul,
    FPUAdd
};

std::string functionalUnitToString(FunctionalUnit unit);

enum class RegisterKind {
    GP,
    FP
};

struct Register {
    RegisterKind kind;
    std::uint64_t number;

    static std::optional<Register> parse(const std::string& text);
    std::string toString() const;
};

enum class OperandKind {
    Immediate,
    Register,
    Indirect,
    Global,
    None
};

struct Operand {
    OperandKind kind = OperandKind::None;

    std::uint64_t immediate = 0;
    std::optional<Register> reg;
    std::string globalName;

    static Operand none();
    static Operand parse(const std::string& text);

    bool isReg() const;
    Register asReg() const;
    std::optional<Register> depReg() const;

    std::string toString() const;
};

enum class RiscVOpKind {
    LoadWord,
    StoreWord,
    LoadFloat,
    StoreFloat,
    Add,
    Sub,
    BranchEqual,
    BranchNotEqual,
    FloatAdd,
    FloatSub,
    FloatMul,
    FloatDiv
};

struct RiscVOp {
    RiscVOpKind kind;

    Operand a;
    Operand b;
    Operand c;

    std::uint64_t memoryAddress = 0;

    static RiscVOp parse(const std::string& line);

    FunctionalUnit functionalUnit() const;

    bool accessesMemory() const;
    bool writesBack() const;

    bool isLoad() const;
    bool isStore() const;
    bool isDataTransfer() const;
    bool isBranch() const;
    bool isAlu() const;
    bool isFp() const;
    bool isFpAdd() const;
    bool isFpMul() const;
    bool isFpDiv() const;

    std::optional<std::uint64_t> addr() const;
    std::optional<Operand> dst() const;

    Operand src1() const;
    Operand src2() const;

    std::string toString() const;
};

std::ostream& operator<<(std::ostream& os, const Register& reg);
std::ostream& operator<<(std::ostream& os, const Operand& operand);
std::ostream& operator<<(std::ostream& os, const RiscVOp& op);

std::vector<RiscVOp> parseInstructionFile(const std::string& filename);

} // namespace Ripes::TomasuloSim

