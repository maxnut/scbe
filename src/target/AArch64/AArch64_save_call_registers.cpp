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

            size_t pos = bb->last() - function->getFunctionPrologueSize();

            for(auto& rr : pushed)
                pos += m_instructionInfo->registersMemoryOp(bb.get(), pos, Opcode::LoadP64rm, {rr, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), 16, Indexing::PostIndexed);
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

size_t AArch64SaveCallRegisters::saveCall(MIR::Block* block, MIR::CallInstruction* instruction) {
    bool changed = false;
    const std::vector<uint32_t>& callerSaved = m_registerInfo->getCallerSavedRegisters();

    std::vector<MIR::Register*> pushed;
    
    size_t inIdx = block->getInstructionIdx(instruction) - instruction->getStartOffset();

    size_t funcIdx = block->getParentFunction()->getInstructionIdx(instruction);
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
        if(!block->getParentFunction()->getRegisterInfo().isRegisterLive(funcIdx, saveReg, m_registerInfo)) continue;
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        // TODO push two registers at a time
        inIdx += m_instructionInfo->registersMemoryOp(block, inIdx, Opcode::StoreP64rm, {argReg, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), -16, Indexing::PreIndexed);
        pushed.push_back(argReg);
        changed = true;
    }

    inIdx = block->getInstructionIdx(instruction) + instruction->getEndOffset();
    std::reverse(pushed.begin(), pushed.end());
    for(auto& rr : pushed)
        inIdx += m_instructionInfo->registersMemoryOp(block, inIdx, Opcode::LoadP64rm, {rr, m_registerInfo->getRegister(XZR)}, m_registerInfo->getRegister(SP), 16, Indexing::PostIndexed);

    return inIdx;
}

}