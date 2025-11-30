#include "target/target_lowering.hpp"
#include "MIR/function.hpp"
#include "target/instruction_info.hpp"

namespace scbe::Target {

bool TargetLowering::run(MIR::Function* function) {
    lowerPhis(function);
    
    for(auto& bb : function->getBlocks()) {
        while(true) {
            bool lower = false;
            for(auto& instr : bb->getInstructions()) {
                if(instr->getOpcode() == CALL_LOWER_OP) {
                    lowerCall(bb.get(), cast<MIR::CallLowering>(instr.get()));
                    lower = true;
                    break;
                }
                else if(instr->getOpcode() == SWITCH_LOWER_OP) {
                    lowerSwitch(bb.get(), cast<MIR::SwitchLowering>(instr.get()));
                    lower = true;
                    break;
                }
                else if(instr->getOpcode() == RETURN_LOWER_OP) {
                    lowerReturn(bb.get(), cast<MIR::ReturnLowering>(instr.get()));
                    lower = true;
                    break;
                }
            }
            if(!lower)
                break;
        }
    }
    lowerFunction(function);

    for(auto& bb : function->getBlocks()) {
        for(auto& instr : bb->getInstructions()) {
            for(auto& op : instr->getOperands()) {
                if(!op || !op->isRegister() || !m_registerInfo->isPhysicalRegister(cast<MIR::Register>(op)->getId())) continue;

                MIR::Register* reg = cast<MIR::Register>(op);
                if(reg->hasFlag(Force64BitRegister)) op = m_registerInfo->getRegister(m_registerInfo->getRegisterWithSize(reg->getId(), 8).value());
                else if(reg->hasFlag(Force32BitRegister)) op = m_registerInfo->getRegister(m_registerInfo->getRegisterWithSize(reg->getId(), 4).value());
                else if(reg->hasFlag(Force16BitRegister)) op = m_registerInfo->getRegister(m_registerInfo->getRegisterWithSize(reg->getId(), 2).value());
                else if(reg->hasFlag(Force8BitRegister)) op = m_registerInfo->getRegister(m_registerInfo->getRegisterWithSize(reg->getId(), 1).value());
            }
        }
    }

    return false;
}

void TargetLowering::lowerPhis(MIR::Function* function) {
    for (auto& block : function->getBlocks()) {
        if (block->getPhiLowering().empty()) continue;

        MIR::Instruction* term = block->getTerminator(m_instructionInfo);
        parallelCopy(block.get());
    }
}


}