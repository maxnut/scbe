#pragma once

#include "calling_convention.hpp"
#include "operand.hpp"
#include "type.hpp"
#include <cstdint>
#include <vector>

#define CALL_LOWER_OP INT_MAX
#define SWITCH_LOWER_OP CALL_LOWER_OP - 1
#define RETURN_LOWER_OP SWITCH_LOWER_OP - 1
#define VA_START_LOWER_OP RETURN_LOWER_OP - 1
#define VA_END_LOWER_OP VA_START_LOWER_OP - 1
#define PHI_LOWER_OP VA_END_LOWER_OP - 1

namespace scbe::MIR {

class Instruction {
public:
    Instruction(uint32_t opcode) : m_opcode(opcode) {}
    template<typename... Args>
    Instruction(uint32_t opcode, Args&&... ops) : m_opcode(opcode) {
        (m_operands.push_back(std::forward<Args>(ops)), ...);
    }

    virtual ~Instruction() = default;

    uint32_t getOpcode() const { return m_opcode; }

    void addOperand(Operand* operand) { m_operands.push_back(operand); }
    std::vector<Operand*>& getOperands() { return m_operands; }
    MIR::Block* getParentBlock() const { return m_parentBlock; }

protected:
    uint32_t m_opcode;
    std::vector<Operand*> m_operands;
    MIR::Block* m_parentBlock = nullptr;

friend class MIR::Block;
};

class CallLowering : public Instruction {
public:
    CallLowering() : Instruction(CALL_LOWER_OP) {}

    void addType(Type* type) { m_types.push_back(type); }
    void setVarArg(bool varArg) { m_isVarArg = varArg; }
    void setCallingConvention(CallingConvention cc) { m_callConv = cc; }

    const std::vector<Type*>& getTypes() const { return m_types; }
    bool isVarArg() const { return m_isVarArg; }
    CallingConvention getCallingConvention() const { return m_callConv; }

private:
    std::vector<Type*> m_types;
    bool m_isVarArg = false;
    CallingConvention m_callConv = CallingConvention::Count;
};

class SwitchLowering : public Instruction {
public:
    SwitchLowering() : Instruction(SWITCH_LOWER_OP) {}

    MIR::Operand* getCondition() { return getOperands().at(0); }
    MIR::Block* getDefault() { return cast<MIR::Block>(getOperands().at(1)); }
    std::vector<std::pair<MIR::ImmediateInt*, MIR::Block*>> getCases() const;
};

class ReturnLowering : public Instruction {
public:
    ReturnLowering() : Instruction(RETURN_LOWER_OP) {}

    MIR::Operand* getValue() { return getOperands().at(0); }
};

class VaStartLowering : public Instruction {
public:
    VaStartLowering() : Instruction(VA_START_LOWER_OP) {}

    Operand* getList() { return getOperands().at(0); }
};

class VaEndLowering : public Instruction {
public:
    VaEndLowering() : Instruction(VA_END_LOWER_OP) {}

    Operand* getList() { return getOperands().at(0); }
};

class PhiLowering : public Instruction {
public:
    PhiLowering() : Instruction(PHI_LOWER_OP) {}
};

class CallInstruction : public Instruction {
public:
    CallInstruction(uint32_t opcode) : Instruction(opcode) {}
    template<typename... Args>
    CallInstruction(uint32_t opcode, Args&&... ops) : Instruction(opcode, std::forward<Args>(ops)...) {}

    MIR::Instruction* getStart() const { return m_start; }
    MIR::Instruction* getEnd() const { return m_end; }

    void setStartOffset(MIR::Instruction* offset) { m_start = offset; }
    void setEndOffset(MIR::Instruction* offset) { m_end = offset; }
    void addReturnRegister(uint32_t reg) { m_returnRegisters.push_back(reg); }

    const std::vector<uint32_t>& getReturnRegisters() const { return m_returnRegisters; }

private:
    MIR::Instruction* m_start = nullptr;
    MIR::Instruction* m_end = nullptr;

    std::vector<uint32_t> m_returnRegisters;
};
    
}