#include "target/x64/x64_target_lowering_pra.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_patterns.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "unit.hpp"
#include "MIR/function.hpp"
#include "IR/function.hpp"

namespace scbe::Target::x64 {

bool x64TargetLoweringPRA::run(MIR::Function* function) {
    TargetLoweringPRA::run(function);

    const std::vector<uint32_t>& calleeSaved = m_registerInfo->getCalleeSavedRegisters();
    std::vector<MIR::Register*> pushed;
    size_t inIdx = function->getFunctionPrologueSize();

    for(uint32_t saveReg : calleeSaved) {
        if(!function->getRegisterInfo().isRegisterEverLive(saveReg, m_registerInfo)) continue;
        if(saveReg == RBP) continue; // we already push this in prologue
        MIR::Register* argReg = m_registerInfo->getRegister(saveReg);
        function->getEntryBlock()->addInstructionAt(instr((uint32_t)Opcode::Push64r, argReg), inIdx++);
        pushed.push_back(argReg);
    }

    if(pushed.size() % 2 != 0)
        function->getEntryBlock()->addInstructionAt(instr((uint32_t)Opcode::Sub64r8i, m_registerInfo->getRegister(RSP), m_ctx->getImmediateInt(8, MIR::ImmediateInt::imm8)), inIdx++);

    std::reverse(pushed.begin(), pushed.end());

    if(pushed.size()) {
        for(auto& bb : function->getBlocks()) {
            if(!bb->hasReturn(m_instructionInfo))
                continue;

            auto ret = bb->getTerminator(m_instructionInfo);
            size_t pos = bb->getInstructionIdx(ret) - bb->getEpilogueSize();

            if(pushed.size() % 2 != 0)
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(RSP), m_ctx->getImmediateInt(8, MIR::ImmediateInt::imm8)), pos++);

            for(auto& rr : pushed)
                bb->addInstructionAt(instr((uint32_t)Opcode::Pop64r, rr), pos++);
        }
    }

    for(auto& instruction : function->getCalls())
        saveCall(instruction);

    return false;
}

void x64TargetLoweringPRA::saveCall(MIR::CallInstruction* instruction) {
    const std::vector<uint32_t>& callerSaved = m_registerInfo->getCallerSavedRegisters();
    MIR::Block* block = instruction->getParentBlock();

    std::vector<MIR::Register*> pushed;

    size_t inIdx = block->getInstructionIdx(instruction->getStart());

    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();

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
        block->addInstructionAt(instr((uint32_t)Opcode::Push64r, argReg), inIdx++);
        pushed.push_back(argReg);
    }

    auto eight = ctx->getImmediateInt(8, MIR::ImmediateInt::imm8);
    if(pushed.size() % 2 != 0)
        block->addInstructionAt(instr((uint32_t)Opcode::Sub64r8i, m_registerInfo->getRegister(RSP), eight), inIdx++);

    size_t blockFnIdx = block->getInstructionIdx(instruction);
    inIdx = blockFnIdx + 1;
    if(pushed.size() % 2 != 0)
        block->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(RSP), eight), inIdx++);

    std::reverse(pushed.begin(), pushed.end());
    for(auto& rr : pushed)
        block->addInstructionAt(instr((uint32_t)Opcode::Pop64r, rr), inIdx++);
}

void x64TargetLoweringPRA::lowerIntrinsic(MIR::Block* block, MIR::IntrinsicLowering* lowering) {
    size_t inIdx = block->getInstructionIdx(lowering);
    size_t fnInIdx = block->getParentFunction()->getInstructionIdx(lowering);
    std::unique_ptr<MIR::IntrinsicLowering> instruction = std::unique_ptr<MIR::IntrinsicLowering>(
        cast<MIR::IntrinsicLowering>(block->removeInstruction(lowering).release())
    );

    MIR::Operand* ret = instruction->getOperands().at(0);

    switch (instruction->getName()) {
        case IntrinsicName::Memcpy: {
            RegisterInfo* ri = m_registerInfo;

            std::array<MIR::Register*, 3> pushed{0};
            if(block->getParentFunction()->getRegisterInfo().isRegisterLive(fnInIdx, RDI, m_registerInfo, false)) {
                pushed[0] = ri->getRegister(RDI);
                block->addInstructionAt(instr((uint32_t)Opcode::Push64r, pushed[0]), inIdx++);
            }
            if(block->getParentFunction()->getRegisterInfo().isRegisterLive(fnInIdx, RDI, m_registerInfo, false)) {
                pushed[1] = ri->getRegister(RSI);
                block->addInstructionAt(instr((uint32_t)Opcode::Push64r, pushed[1]), inIdx++);
            }
            if(block->getParentFunction()->getRegisterInfo().isRegisterLive(fnInIdx, RDI, m_registerInfo, false)) {
                pushed[2] = ri->getRegister(RCX);
                block->addInstructionAt(instr((uint32_t)Opcode::Push64r, pushed[2]), inIdx++);
            }

            MIR::Operand* dst = instruction->getOperands().at(1);
            MIR::Operand* src = instruction->getOperands().at(2);
            MIR::Operand* len = instruction->getOperands().at(3);
            x64InstructionInfo* x64Info = (x64InstructionInfo*)m_instructionInfo;
            MIR::StackFrame& frame = block->getParentFunction()->getStackFrame();
            // assume size 8 and no float since we should always have pointers
            if(dst->isFrameIndex())
                inIdx += x64Info->stackSlotAddress(block, inIdx, frame.getStackSlot(cast<MIR::FrameIndex>(dst)->getIndex()), ri->getRegister(RDI));
            else
                inIdx += m_instructionInfo->move(block, inIdx, dst, ri->getRegister(RDI), 8, false);

            if(src->isFrameIndex())
                inIdx += x64Info->stackSlotAddress(block, inIdx, frame.getStackSlot(cast<MIR::FrameIndex>(src)->getIndex()), ri->getRegister(RSI));
            else
                inIdx += m_instructionInfo->move(block, inIdx, src, ri->getRegister(RSI), 8, false);

            inIdx += m_instructionInfo->move(block, inIdx, len, ri->getRegister(RCX), 8, false);

            block->addInstructionAt(instr(OPCODE(Rep_Movsb)), inIdx++);

            for(int i = 2; i >= 0; i--) {
                if(!pushed[i]) continue;
                block->addInstructionAt(instr((uint32_t)Opcode::Pop64r, pushed[i]), inIdx++);
            }

            break;
        }
        default: break;
    }
}

}