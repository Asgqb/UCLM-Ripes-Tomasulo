#include "tomasulo_op.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Ripes::TomasuloSim {

static std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

static bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

static bool parseUnsigned(const std::string& text, std::uint64_t& value, int base = 10) {
    try {
        std::size_t pos = 0;
        value = std::stoull(text, &pos, base);
        return pos == text.size();
    } catch (...) {
        return false;
    }
}

static std::vector<std::string> splitByComma(const std::string& text) {
    std::vector<std::string> result;
    std::stringstream ss(text);
    std::string item;

    while (std::getline(ss, item, ',')) {
        result.push_back(trim(item));
    }

    return result;
}

std::string functionalUnitToString(FunctionalUnit unit) {
    switch (unit) {
        case FunctionalUnit::ALU:
            return "ALU";
        case FunctionalUnit::EffectAddr:
            return "EffectAddr";
        case FunctionalUnit::FPUMul:
            return "FPUMul";
        case FunctionalUnit::FPUAdd:
            return "FPUAdd";
    }

    return "Unknown";
}

std::optional<Register> Register::parse(const std::string& text) {
    const std::string regText = trim(text);

    if (regText.size() < 2) {
        return std::nullopt;
    }

    const char prefix = regText[0];
    const std::string numberText = regText.substr(1);

    std::uint64_t number = 0;
    if (!parseUnsigned(numberText, number)) {
        return std::nullopt;
    }

    if (prefix == 'x') {
        return Register{RegisterKind::GP, number};
    }

    if (prefix == 'f') {
        return Register{RegisterKind::FP, number};
    }

    return std::nullopt;
}

std::string Register::toString() const {
    if (kind == RegisterKind::GP) {
        return "x" + std::to_string(number);
    }

    return "f" + std::to_string(number);
}

Operand Operand::none() {
    return Operand{};
}

Operand Operand::parse(const std::string& text) {
    const std::string value = trim(text);

    if (value.empty()) {
        return Operand::none();
    }

    if (auto reg = Register::parse(value)) {
        Operand operand;
        operand.kind = OperandKind::Register;
        operand.reg = reg;
        return operand;
    }

    if (startsWith(value, "0x")) {
        std::uint64_t imm = 0;
        if (parseUnsigned(value.substr(2), imm, 16)) {
            Operand operand;
            operand.kind = OperandKind::Immediate;
            operand.immediate = imm;
            return operand;
        }
    }

    std::uint64_t imm = 0;
    if (parseUnsigned(value, imm)) {
        Operand operand;
        operand.kind = OperandKind::Immediate;
        operand.immediate = imm;
        return operand;
    }

    const auto openParen = value.find('(');
    const auto closeParen = value.find(')');

    if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen) {
        const std::string offsetText = trim(value.substr(0, openParen));
        const std::string regText = trim(value.substr(openParen + 1, closeParen - openParen - 1));

        std::uint64_t offset = 0;
        if (!parseUnsigned(offsetText, offset)) {
            throw std::runtime_error("Could not parse offset: " + offsetText);
        }

        auto reg = Register::parse(regText);
        if (!reg) {
            throw std::runtime_error("Could not parse register: " + regText);
        }

        Operand operand;
        operand.kind = OperandKind::Indirect;
        operand.immediate = offset;
        operand.reg = reg;
        return operand;
    }

    Operand operand;
    operand.kind = OperandKind::Global;
    operand.globalName = value;
    return operand;
}

bool Operand::isReg() const {
    return kind == OperandKind::Register;
}

Register Operand::asReg() const {
    if (!reg) {
        throw std::runtime_error("Operand is not a register: " + toString());
    }

    return *reg;
}

std::optional<Register> Operand::depReg() const {
    if (kind == OperandKind::Register || kind == OperandKind::Indirect) {
        return reg;
    }

    return std::nullopt;
}

std::string Operand::toString() const {
    switch (kind) {
        case OperandKind::Immediate:
            return std::to_string(immediate);

        case OperandKind::Register:
            return reg ? reg->toString() : "";

        case OperandKind::Indirect:
            return std::to_string(immediate) + "(" + reg->toString() + ")";

        case OperandKind::Global:
            return globalName;

        case OperandKind::None:
            return "";
    }

    return "";
}

RiscVOp RiscVOp::parse(const std::string& originalLine) {
    std::string line = originalLine;

    const auto commentPos = line.find(";;");
    if (commentPos != std::string::npos) {
        line = line.substr(0, commentPos);
    }

    line = trim(line);

    if (line.empty()) {
        throw std::runtime_error("Cannot parse empty instruction");
    }

    std::uint64_t address = 0;

    const auto colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        const std::string addrText = trim(line.substr(colonPos + 1));
        if (!parseUnsigned(addrText, address)) {
            throw std::runtime_error("Could not parse memory address: " + addrText);
        }

        line = trim(line.substr(0, colonPos));
    }

    std::stringstream ss(line);

    std::string opName;
    ss >> opName;

    std::string rest;
    std::getline(ss, rest);
    rest = trim(rest);

    const auto args = splitByComma(rest);

    auto getArg = [&](std::size_t index) -> Operand {
        if (index >= args.size()) {
            throw std::runtime_error("Missing argument in instruction: " + originalLine);
        }

        return Operand::parse(args[index]);
    };

    if (opName == "add") {
        return RiscVOp{RiscVOpKind::Add, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "sub") {
        return RiscVOp{RiscVOpKind::Sub, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "lw") {
        return RiscVOp{RiscVOpKind::LoadWord, getArg(0), getArg(1), Operand::none(), address};
    }

    if (opName == "sw") {
        return RiscVOp{RiscVOpKind::StoreWord, getArg(0), getArg(1), Operand::none(), address};
    }

    if (opName == "beq") {
        return RiscVOp{RiscVOpKind::BranchEqual, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "bne") {
        return RiscVOp{RiscVOpKind::BranchNotEqual, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "flw") {
        return RiscVOp{RiscVOpKind::LoadFloat, getArg(0), getArg(1), Operand::none(), address};
    }

    if (opName == "fsw") {
        return RiscVOp{RiscVOpKind::StoreFloat, getArg(0), getArg(1), Operand::none(), address};
    }

    if (opName == "fadd" || opName == "fadd.s") {
        return RiscVOp{RiscVOpKind::FloatAdd, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "fsub" || opName == "fsub.s") {
        return RiscVOp{RiscVOpKind::FloatSub, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "fmul" || opName == "fmul.s") {
        return RiscVOp{RiscVOpKind::FloatMul, getArg(0), getArg(1), getArg(2), address};
    }

    if (opName == "fdiv" || opName == "fdiv.s") {
        return RiscVOp{RiscVOpKind::FloatDiv, getArg(0), getArg(1), getArg(2), address};
    }

    throw std::runtime_error("Unknown instruction: " + opName);
}

FunctionalUnit RiscVOp::functionalUnit() const {
    if (isBranch()) {
        return FunctionalUnit::EffectAddr;
    }

    if (isAlu()) {
        return FunctionalUnit::ALU;
    }

    if (isFpAdd()) {
        return FunctionalUnit::FPUAdd;
    }

    if (isFpMul()) {
        return FunctionalUnit::FPUMul;
    }

    if (isDataTransfer()) {
        return FunctionalUnit::EffectAddr;
    }

    throw std::runtime_error("Unknown functional unit for operation: " + toString());
}

bool RiscVOp::accessesMemory() const {
    return isLoad();
}

bool RiscVOp::writesBack() const {
    return isAlu() || isFp() || isLoad();
}

bool RiscVOp::isLoad() const {
    return kind == RiscVOpKind::LoadWord || kind == RiscVOpKind::LoadFloat;
}

bool RiscVOp::isStore() const {
    return kind == RiscVOpKind::StoreWord || kind == RiscVOpKind::StoreFloat;
}

bool RiscVOp::isDataTransfer() const {
    return isLoad() || isStore();
}

bool RiscVOp::isBranch() const {
    return kind == RiscVOpKind::BranchEqual || kind == RiscVOpKind::BranchNotEqual;
}

bool RiscVOp::isAlu() const {
    return kind == RiscVOpKind::Add || kind == RiscVOpKind::Sub;
}

bool RiscVOp::isFp() const {
    return kind == RiscVOpKind::FloatAdd ||
           kind == RiscVOpKind::FloatSub ||
           kind == RiscVOpKind::FloatMul ||
           kind == RiscVOpKind::FloatDiv;
}

bool RiscVOp::isFpAdd() const {
    return kind == RiscVOpKind::FloatAdd || kind == RiscVOpKind::FloatSub;
}

bool RiscVOp::isFpMul() const {
    return kind == RiscVOpKind::FloatMul || kind == RiscVOpKind::FloatDiv;
}

bool RiscVOp::isFpDiv() const {
    return kind == RiscVOpKind::FloatDiv;
}

std::optional<std::uint64_t> RiscVOp::addr() const {
    if (isDataTransfer()) {
        return memoryAddress;
    }

    return std::nullopt;
}

std::optional<Operand> RiscVOp::dst() const {
    if (isStore() || isBranch()) {
        return std::nullopt;
    }

    return a;
}

Operand RiscVOp::src1() const {
    switch (kind) {
        case RiscVOpKind::LoadWord:
        case RiscVOpKind::LoadFloat:
            return b;

        case RiscVOpKind::StoreWord:
        case RiscVOpKind::StoreFloat:
            return a;

        case RiscVOpKind::Add:
        case RiscVOpKind::Sub:
        case RiscVOpKind::FloatAdd:
        case RiscVOpKind::FloatSub:
        case RiscVOpKind::FloatMul:
        case RiscVOpKind::FloatDiv:
            return b;

        case RiscVOpKind::BranchEqual:
        case RiscVOpKind::BranchNotEqual:
            return a;
    }

    return Operand::none();
}

Operand RiscVOp::src2() const {
    switch (kind) {
        case RiscVOpKind::LoadWord:
        case RiscVOpKind::LoadFloat:
            return Operand::none();

        case RiscVOpKind::StoreWord:
        case RiscVOpKind::StoreFloat:
            return b;

        case RiscVOpKind::Add:
        case RiscVOpKind::Sub:
        case RiscVOpKind::FloatAdd:
        case RiscVOpKind::FloatSub:
        case RiscVOpKind::FloatMul:
        case RiscVOpKind::FloatDiv:
            return c;

        case RiscVOpKind::BranchEqual:
        case RiscVOpKind::BranchNotEqual:
            return b;
    }

    return Operand::none();
}

std::string RiscVOp::toString() const {
    switch (kind) {
        case RiscVOpKind::LoadWord:
            return "lw " + a.toString() + "," + b.toString() + ":" + std::to_string(memoryAddress);

        case RiscVOpKind::StoreWord:
            return "sw " + a.toString() + "," + b.toString() + ":" + std::to_string(memoryAddress);

        case RiscVOpKind::LoadFloat:
            return "flw " + a.toString() + "," + b.toString() + ":" + std::to_string(memoryAddress);

        case RiscVOpKind::StoreFloat:
            return "fsw " + a.toString() + "," + b.toString() + ":" + std::to_string(memoryAddress);

        case RiscVOpKind::Add:
            return "add " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::Sub:
            return "sub " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::BranchEqual:
            return "beq " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::BranchNotEqual:
            return "bne " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::FloatAdd:
            return "fadd.s " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::FloatSub:
            return "fsub.s " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::FloatMul:
            return "fmul.s " + a.toString() + "," + b.toString() + "," + c.toString();

        case RiscVOpKind::FloatDiv:
            return "fdiv.s " + a.toString() + "," + b.toString() + "," + c.toString();
    }

    return "";
}

std::ostream& operator<<(std::ostream& os, const Register& reg) {
    os << reg.toString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const Operand& operand) {
    os << operand.toString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const RiscVOp& op) {
    os << op.toString();
    return os;
}

std::vector<RiscVOp> parseInstructionFile(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open trace file: " + filename);
    }

    std::vector<RiscVOp> instructions;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty()) {
            continue;
        }

        instructions.push_back(RiscVOp::parse(line));
    }

    return instructions;
}

} // namespace Ripes::TomasuloSim

