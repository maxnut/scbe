#pragma once

#include "ISel/DAG/node.hpp"
#include "ISel/DAG/pattern.hpp"
#include "MIR/block.hpp"
#include "MIR/operand.hpp"
#include "MIR/stack_slot.hpp"
#include "target/register_info.hpp"
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <unordered_map>
#include <vector>

namespace scbe::Target {

class Restriction {
public:
    Restriction(const std::initializer_list<uint32_t> kinds, bool assigned = false) : m_assigned(assigned) {
        for (auto kind : kinds) m_kindBitmask |= 1 << (uint32_t)kind;
    }

    Restriction(uint32_t bitmask, bool assigned) : m_kindBitmask(bitmask), m_assigned(assigned) {}

    bool isAllowed(uint32_t kind) const { return (m_kindBitmask & (1 << kind)) != 0; }
    bool isAssigned() const { return m_assigned; }

    static Restriction reg(bool assigned = false) { return Restriction(1 << (uint32_t)MIR::Operand::Kind::Register, assigned); }
    static Restriction imm(bool assigned = false) { return Restriction(1 << (uint32_t)MIR::Operand::Kind::ImmediateInt, assigned); }
    static Restriction sym() { return Restriction({(uint32_t)MIR::Operand::Kind::Block, (uint32_t)MIR::Operand::Kind::GlobalAddress, (uint32_t)MIR::Operand::Kind::ExternalSymbol, (uint32_t)MIR::Operand::Kind::ConstantIndex}, false); }

private:
    uint32_t m_kindBitmask;
    bool m_assigned = false;
};

class InstructionDescriptor {
public:
    InstructionDescriptor() = default;

    InstructionDescriptor(std::string name, size_t numDefs, size_t numOperands, size_t size, bool mayStore, bool mayLoad, const std::vector<Restriction>& operandRestrictions, const std::vector<uint32_t>& clobberRegisters = {}, bool isReturn = false, bool isJump = false) : m_name(name), m_numDefs(numDefs), m_numOperands(numOperands), m_size(size), m_mayStore(mayStore), m_mayLoad(mayLoad), m_isReturn(isReturn), m_operandRestrictions(operandRestrictions), m_clobberRegisters(clobberRegisters), m_isJump(isJump) {}

    const std::string& getName() const { return m_name; }
    size_t getNumDefs() const { return m_numDefs; }
    size_t getNumOperands() const { return m_numOperands; }
    size_t getSize() const { return m_size; }

    bool mayStore() const { return m_mayStore; }
    bool mayLoad() const { return m_mayLoad; }

    bool isReturn() const { return m_isReturn; }
    bool isJump() const { return m_isJump; }

    const Restriction& getRestriction(size_t idx) const { return m_operandRestrictions[idx]; }
    const std::vector<uint32_t>& getClobberRegisters() const { return m_clobberRegisters; }

protected:
    std::string m_name;
    size_t m_numDefs;
    size_t m_numOperands;
    size_t m_size = 0; // in bytes

    bool m_mayStore;
    bool m_mayLoad;
    bool m_isReturn;
    bool m_isJump;

    std::vector<Restriction> m_operandRestrictions;
    std::vector<uint32_t> m_clobberRegisters;
}; 

class Mnemonic {
public:
    Mnemonic() {}
    Mnemonic(std::string name) : m_name(name) {}
    const std::string& getName() const { return m_name; }
protected:
    std::string m_name;
};

enum OperandFlags : uint32_t {
    Force64BitRegister = 1 << 0,
    Force32BitRegister = 1 << 1,
    Force16BitRegister = 1 << 2,
    Force8BitRegister = 1 << 3,
    Count = 4
};

class InstructionInfo {
public:
    InstructionInfo(RegisterInfo* registerInfo, Ref<Context> ctx) : m_registerInfo(registerInfo), m_ctx(ctx) {}

    const InstructionDescriptor& getInstructionDescriptor(uint32_t opcode) {
        if(opcode == CALL_LOWER_OP) {
            static InstructionDescriptor descriptor("CallLower", 0, 0, 0, false, false, {});
            return descriptor;
        }
        else if(opcode == SWITCH_LOWER_OP) {
            static InstructionDescriptor descriptor("SwitchLower", 0, 0, 0, false, false, {});
            return descriptor;
        }
        else if(opcode == RETURN_LOWER_OP) {
            static InstructionDescriptor descriptor("SwitchLower", 0, 0, 0, false, false, {}, {}, true);
            return descriptor;
        }
        assert(opcode < m_instructionDescriptors.size() && "Invalid opcode for descriptor");
        return m_instructionDescriptors.at(opcode);
    }

    Mnemonic getMnemonic(uint32_t opcode) {
        assert(opcode < m_mnemonics.size() && "Invalid opcode for mnemonic");
        return m_mnemonics[opcode];
    }

    RegisterInfo* getRegisterInfo() { return m_registerInfo; }

    virtual const std::vector<ISel::DAG::Pattern>& getPatterns(ISel::DAG::Node::NodeKind kind) {
        return m_patterns[kind];
    }

    virtual bool isReturn(uint32_t opcode) { return m_instructionDescriptors[opcode].isReturn(); }

    virtual size_t registerToStackSlot(MIR::Block* block, size_t pos, MIR::Register* reg, MIR::StackSlot slot) = 0;
    virtual size_t stackSlotToRegister(MIR::Block* block, size_t pos, MIR::Register* reg, MIR::StackSlot slot) = 0;
    virtual size_t immediateToStackSlot(MIR::Block* block, size_t pos, MIR::ImmediateInt* immediate, MIR::StackSlot stackSlot) = 0;
    virtual size_t move(MIR::Block* block, size_t pos, MIR::Operand* source, MIR::Operand* destination, size_t size, bool flt) = 0;

protected:
    InstructionInfo() {}

    std::vector<InstructionDescriptor> m_instructionDescriptors;
    std::vector<Mnemonic> m_mnemonics;
    std::unordered_map<ISel::DAG::Node::NodeKind, std::vector<ISel::DAG::Pattern>> m_patterns;
    RegisterInfo* m_registerInfo = nullptr;
    Ref<Context> m_ctx = nullptr;
};

}