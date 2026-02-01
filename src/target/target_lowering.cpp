#include "target/target_lowering.hpp"
#include "MIR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/register_info.hpp"
#include "target/instruction_info.hpp"
#include "type_alias.hpp"

namespace scbe::Target {

bool TargetLowering::run(MIR::Function* function) {
    lowerPhis(function);
    
    for(auto& bb : function->getBlocks()) {
        while(true) {
            bool lower = false;
            for(auto& instr : bb->getInstructions()) {
                if(instr->getOpcode() == CALL_LOWER_OP) {
                    function->addCall(lowerCall(bb.get(), cast<MIR::CallLowering>(instr.get())));
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
        while(true) {
            bool lower = false;
            for(auto& instr : bb->getInstructions()) {
                if(instr->getOpcode() == VA_START_LOWER_OP) {
                    lowerVaStart(bb.get(), cast<MIR::VaStartLowering>(instr.get()));
                    lower = true;
                    break;
                }
                else if(instr->getOpcode() == VA_END_LOWER_OP) {
                    lowerVaEnd(bb.get(), cast<MIR::VaEndLowering>(instr.get()));
                    lower = true;
                    break;
                }
            }
            if(!lower)
                break;
        }
    }

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
        if (block->getPhis().empty()) continue;

        for(MIR::Block* pred : block->getPredecessors()) {
            std::vector<std::pair<MIR::Register*, MIR::Operand*>> copies;
            
            for(auto& phi : block->getPhis()) {
                MIR::Register* dst = cast<MIR::Register>(phi->getOperands().at(0));
                for(size_t i = 1; i < phi->getOperands().size(); i += 2) {
                    if(phi->getOperands().at(i+1) != pred) continue;
                    copies.push_back({dst, phi->getOperands().at(i)});
                }
            }

            if(copies.empty()) continue;

            assert(pred->getSuccessors().size() == 1);

            parallelCopy(copies, pred);
        }

        for(auto& phi : block->getPhis()) block->removeInstruction(phi);
    }
}

void TargetLowering::parallelCopy(std::vector<std::pair<MIR::Register*, MIR::Operand*>>& copies, MIR::Block* block) {
    std::vector<MIR::Instruction*> allJumpsTo;
    for(auto& ins : block->getInstructions()) {
        if(!m_instructionInfo->getInstructionDescriptor(ins->getOpcode()).isJump()) continue;
        allJumpsTo.push_back(ins.get());
    }

    USet<MIR::Operand*> destinations;
    for(auto& copy : copies) destinations.insert(copy.first);

    auto emitMove = [&](MIR::Register* dst, MIR::Operand* src) {
        for(MIR::Instruction* jmp : allJumpsTo) {
            size_t idx = block->getInstructionIdx(jmp);
            MIR::VRegInfo info = block->getParentFunction()->getRegisterInfo().getVirtualRegisterInfo(dst->getId());
            size_t size = info.m_typeOverride ? m_dataLayout->getSize(info.m_typeOverride) : 
                m_registerInfo->getRegisterClass(info.m_class).getSize();
            m_instructionInfo->move(block, idx, src, dst, size, m_registerInfo->isFPR(info.m_class));
        }
    };

    while(!copies.empty()) {
        bool acyclic = false;

        for(size_t i = 0; i < copies.size(); i++) {
            auto& copy = copies.at(i);
            if(destinations.contains(copy.second)) continue;

            emitMove(copy.first, copy.second);
            copies.erase(copies.begin() + i);
            acyclic = true;
            break;
        }

        if(acyclic) continue;

        auto& copy = copies.front();
        MIR::VRegInfo info = block->getParentFunction()->getRegisterInfo().getVirtualRegisterInfo(copy.first->getId());
        MIR::Register* vreg = m_registerInfo->getRegister(
            block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(info.m_class, info.m_typeOverride)
        );

        emitMove(vreg, copy.second);

        for (auto& other : copies) {
            if (other.second == copy.second) other.second = vreg;
        }
    }
}


}