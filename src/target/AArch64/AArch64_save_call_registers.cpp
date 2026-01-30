#include "target/AArch64/AArch64_save_call_registers.hpp"
#include "target/AArch64/AArch64_instruction_info.hpp"
#include "target/AArch64/AArch64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "cast.hpp"
#include "MIR/function.hpp"

namespace scbe::Target::AArch64 {

bool AArch64SaveCallRegisters::run(MIR::Function* function) {
    const std::vector<uint32_t>& calleeSaved = m_registerInfo->getCalleeSavedRegisters();
    std::vector<MIR::Register*> pushed;
    size_t inIdx = function->getFunctionPrologueSize();

    for(uint32_t saveReg : calleeSaved) {
        if(!function->getRegisterInfo().isRegisterEverLive(saveReg, m_registerInfo)) continue;
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        // TODO push two registers at a time
        inIdx += m_instructionInfo->registersMemoryOp(function->getEntryBlock(), inIdx, Opcode::StoreP64rm, {argReg, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), -16, Indexing::PreIndexed);
        pushed.push_back(argReg);
    }

    std::reverse(pushed.begin(), pushed.end());

    if(pushed.size()) {
        for(auto& bb : function->getBlocks()) {
            if(!bb->hasReturn(m_instructionInfo))
                continue;

            auto ret = bb->getTerminator(m_instructionInfo);
            size_t pos = bb->getInstructionIdx(ret) - bb->getEpilogueSize();

            for(auto& rr : pushed)
                pos += m_instructionInfo->registersMemoryOp(bb.get(), pos, Opcode::LoadP64rm, {rr, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), 16, Indexing::PostIndexed);
        }
    }

    for(auto& instruction : function->getCalls())
        saveCall(instruction);

    return false;
}

void AArch64SaveCallRegisters::saveCall(MIR::CallInstruction* instruction) {
    bool changed = false;
    const std::vector<uint32_t>& callerSaved = m_registerInfo->getCallerSavedRegisters();
    MIR::Block* block = instruction->getParentBlock();

    std::vector<MIR::Register*> pushed;
    
    size_t inIdx = block->getInstructionIdx(instruction) - instruction->getStartOffset();

    for(uint32_t saveReg : callerSaved) {
        bool isReturnReg = false;
        for(uint32_t retReg : instruction->getReturnRegisters()) {
            if(m_registerInfo->isSameRegister(saveReg, retReg)) {
                isReturnReg = true;
                break;
            }
        }
        if(isReturnReg) continue;
        // + 1 here because we only care to save it stuff AFTER the call uses the register
        // if for example we are trying to save r8 and the instruction is call r8, and nothing else
        // uses r8 after there's no point in saving it
        size_t callFnIdx = block->getParentFunction()->getInstructionIdx(instruction) + 1;
        if(!block->getParentFunction()->getRegisterInfo().isRegisterLive(callFnIdx, saveReg, m_registerInfo, false)) continue;
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        // TODO push two registers at a time
        inIdx += m_instructionInfo->registersMemoryOp(block, inIdx, Opcode::StoreP64rm, {argReg, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), -16, Indexing::PreIndexed);
        pushed.push_back(argReg);
        changed = true;
    }

    inIdx = block->getInstructionIdx(instruction) + 1;
    std::reverse(pushed.begin(), pushed.end());
    for(auto& rr : pushed)
        inIdx += m_instructionInfo->registersMemoryOp(block, inIdx, Opcode::LoadP64rm, {rr, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), 16, Indexing::PostIndexed);
}

}