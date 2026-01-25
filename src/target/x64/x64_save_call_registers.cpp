#include "target/x64/x64_save_call_registers.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "unit.hpp"
#include "MIR/function.hpp"
#include "IR/function.hpp"

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

            auto ret = bb->getTerminator(m_instructionInfo);
            size_t pos = bb->getInstructionIdx(ret) - bb->getEpilogueSize();

            if(pushed.size() % 2 != 0)
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(RSP), ctx->getImmediateInt(8, MIR::ImmediateInt::imm8)), pos++);

            for(auto& rr : pushed)
                bb->addInstructionAt(instr((uint32_t)Opcode::Pop64r, rr), pos++);
        }
    }

    for(auto& instruction : function->getCalls())
        saveCall(instruction);

    return false;
}

void x64SaveCallRegisters::saveCall(MIR::CallInstruction* instruction) {
    const std::vector<uint32_t>& callerSaved = m_registerInfo->getCallerSavedRegisters();
    MIR::Block* block = instruction->getParentBlock();

    std::vector<MIR::Register*> pushed;

    size_t blockFnIdx = block->getInstructionIdx(instruction);
    
    size_t inIdx = blockFnIdx - instruction->getStartOffset();

    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();

    size_t callFnIdx = block->getParentFunction()->getInstructionIdx(instruction);
    for(uint32_t saveReg : callerSaved) {
        bool isReturnReg = false;
        for(uint32_t retReg : instruction->getReturnRegisters()) {
            if(m_registerInfo->isSameRegister(saveReg, retReg)) {
                isReturnReg = true;
                break;
            }
        }
        if(isReturnReg) continue;
        if(!block->getParentFunction()->getRegisterInfo().isRegisterLive(callFnIdx, saveReg, m_registerInfo)) continue;
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        block->addInstructionAt(instr((uint32_t)Opcode::Push64r, argReg), inIdx++);
        pushed.push_back(argReg);
    }

    auto eight = ctx->getImmediateInt(8, MIR::ImmediateInt::imm8);
    if(pushed.size() % 2 != 0)
        block->addInstructionAt(instr((uint32_t)Opcode::Sub64r8i, m_registerInfo->getRegister(RSP), eight), inIdx++);

    blockFnIdx = block->getInstructionIdx(instruction);
    inIdx = blockFnIdx + 1;
    if(pushed.size() % 2 != 0)
        block->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(RSP), eight), inIdx++);

    std::reverse(pushed.begin(), pushed.end());
    for(auto& rr : pushed)
        block->addInstructionAt(instr((uint32_t)Opcode::Pop64r, rr), inIdx++);
}

}