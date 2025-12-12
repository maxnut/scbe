#include "target/x64/x64_save_call_registers.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "cast.hpp"
#include "unit.hpp"
#include "MIR/function.hpp"
#include "IR/function.hpp"
#include <iostream>

namespace scbe::Target::x64 {

bool x64SaveCallRegisters::run(MIR::Function* function) {
    const std::vector<uint32_t>& calleeSaved = m_registerInfo->getCalleeSavedRegisters();
    std::vector<MIR::Register*> pushed;
    size_t inIdx = function->getFunctionPrologueSize();

    Ref<Context> ctx = function->getIRFunction()->getUnit()->getContext();

    for(uint32_t saveReg : calleeSaved) {
        if(!function->getRegisterInfo().isRegisterEverLive(saveReg, m_registerInfo)) continue;
        if(saveReg == RBP) continue; // we already push this in prologue
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        function->getEntryBlock()->addInstructionAt(instr((uint32_t)Opcode::Push64r, argReg), inIdx++);
        pushed.push_back(argReg);
    }

    if(pushed.size() % 2 != 0)
        function->getEntryBlock()->addInstructionAt(instr((uint32_t)Opcode::Sub64r8i, m_registerInfo->getRegister(RSP), ctx->getImmediateInt(8, MIR::ImmediateInt::imm8)), inIdx++);

    std::reverse(pushed.begin(), pushed.end());

    if(pushed.size()) {
        for(auto& bb : function->getBlocks()) {
            if(!bb->hasReturn(m_instructionInfo))
                continue;

            size_t pos = bb->last() - function->getFunctionPrologueSize();

            if(pushed.size() % 2 != 0)
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(RSP), ctx->getImmediateInt(8, MIR::ImmediateInt::imm8)), pos++);

            for(auto& rr : pushed)
                bb->addInstructionAt(instr((uint32_t)Opcode::Pop64r, rr), pos++);
        }
    }

    for(auto& block : function->getBlocks()) {
        for(size_t i = 0; i < block->getInstructions().size(); i++) {
            auto& instruction = block->getInstructions().at(i);
            if(instruction->getOpcode() != (uint32_t)Opcode::Call && instruction->getOpcode() != (uint32_t)Opcode::Call64r) continue;
            i = saveCall(block.get(), cast<MIR::CallInstruction>(instruction.get()));
        }
    }

    return false;
}

size_t x64SaveCallRegisters::saveCall(MIR::Block* block, MIR::CallInstruction* instruction) {
    const std::vector<uint32_t>& callerSaved = m_registerInfo->getCallerSavedRegisters();

    std::vector<MIR::Register*> pushed;
    
    size_t inIdx = block->getInstructionIdx(instruction) - instruction->getStartOffset();

    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();

    size_t callIdx = block->getParentFunction()->getInstructionIdx(instruction);
    for(uint32_t saveReg : callerSaved) {
        bool isReturnReg = false;
        for(uint32_t retReg : instruction->getReturnRegisters()) {
            if(m_registerInfo->isSameRegister(saveReg, retReg)) {
                isReturnReg = true;
                break;
            }
        }
        if(isReturnReg) continue;
        // TODO: i'm pretty sure this is wrong because i don't recalculate live ranges AFTER regalloc. needs fix
        if(!block->getParentFunction()->getRegisterInfo().isRegisterLive(callIdx, saveReg, m_registerInfo)) continue;
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        block->addInstructionAt(instr((uint32_t)Opcode::Push64r, argReg), inIdx++);
        pushed.push_back(argReg);
    }

    auto eight = ctx->getImmediateInt(8, MIR::ImmediateInt::imm8);
    if(pushed.size() % 2 != 0)
        block->addInstructionAt(instr((uint32_t)Opcode::Sub64r8i, m_registerInfo->getRegister(RSP), eight), inIdx++);

    inIdx = block->getInstructionIdx(instruction) + instruction->getEndOffset();
    if(pushed.size() % 2 != 0)
        block->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(RSP), eight), inIdx++);

    std::reverse(pushed.begin(), pushed.end());
    for(auto& rr : pushed)
        block->addInstructionAt(instr((uint32_t)Opcode::Pop64r, rr), inIdx++);

    return inIdx;
}

}