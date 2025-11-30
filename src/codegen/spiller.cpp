#include "codegen/spiller.hpp"
#include "MIR/function.hpp"
#include "MIR/register_info.hpp"
#include "MIR/stack_slot.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include <memory>

namespace scbe::Codegen {

void Spiller::spill(MIR::Register* replace, MIR::Function* function) {
    const Target::RegisterClass& rclass = m_registerInfo->getRegisterClass(function->getRegisterInfo().getVirtualRegisterInfo(replace->getId()).m_class);
    function->getStackFrame().addStackSlot(rclass.getSize(), rclass.getAlignment());
    spill(replace, function, function->getStackFrame().getStackSlot(function->getStackFrame().getNumStackSlots() - 1));
}

void Spiller::spill(MIR::Register* replace, MIR::Function* function, MIR::StackSlot slot) {
    for(auto& block : function->getBlocks()) {
        bool change = false;
        do {
            change = false;
            for(size_t i = 0; i < block->getInstructions().size(); i++) {
                auto& inst = block->getInstructions().at(i);
                const Target::InstructionDescriptor& desc = m_instructionInfo->getInstructionDescriptor(inst->getOpcode());
                for(size_t j = 0; j < inst->getOperands().size(); j++) {
                    MIR::Operand*& op = inst->getOperands().at(j);
                    // if(op != replace) continue;
                    if(!op || !op->equals(replace, true)) continue;
                    MIR::Register* reg = cast<MIR::Register>(op);

                    change = true;
                    MIR::VRegInfo info = function->getRegisterInfo().getVirtualRegisterInfo(reg->getId());
                    
                    auto rr = m_registerInfo->getRegister(function->getRegisterInfo().getNextVirtualRegister(info.m_class));
                    op = rr;

                    if(desc.getRestriction(j).isAssigned()) {
                        m_instructionInfo->registerToStackSlot(block.get(), i + 1, rr, slot);
                        break;
                    }

                    m_instructionInfo->stackSlotToRegister(block.get(), i, rr, slot);
                    break;
                }
                if(change) break;
            }
        } while(change);
    }
}

}