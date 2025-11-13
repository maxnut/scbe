#include "codegen/regalloc_base.hpp"
#include "MIR/function.hpp"
#include "target/instruction_info.hpp"
#include "target/instruction_utils.hpp"

#include <limits>

namespace scbe::Codegen {

bool RegallocBase::run(MIR::Function* function) {
    do {
        processSpills(function);
        analyze(function);
    }
    while(function->getRegisterInfo().getSpills().size() > 0);

    for(auto& block : function->getBlocks()) {
        bool change = false;
        do {
            change = false;
            for(auto& instr : block->getInstructions()) {
                for(auto& op : instr->getOperands()) {
                    if(!op || op->getKind() != MIR::Operand::Kind::Register) continue;
                    MIR::Register* reg = cast<MIR::Register>(op);
                    if(m_registerInfo->isPhysicalRegister(reg->getId())) continue;

                    uint32_t rr = pickAvailableRegister(function, reg->getId());
                    if(rr == SPILL) throw std::runtime_error("Failed to allocate register");

                    change = true;
                    
                    if(op->hasFlag(Target::Force64BitRegister)) {
                        rr = m_registerInfo->getRegisterWithSize(rr, 8).value();
                    }
                    else if(op->hasFlag(Target::Force32BitRegister)) {
                        rr = m_registerInfo->getRegisterWithSize(rr, 4).value();
                    }
                    else if(op->hasFlag(Target::Force16BitRegister)) {
                        rr = m_registerInfo->getRegisterWithSize(rr, 2).value();
                    }
                    else if(op->hasFlag(Target::Force8BitRegister)) {
                        rr = m_registerInfo->getRegisterWithSize(rr, 1).value();
                    }

                    function->getRegisterInfo().addPVMapping(rr, reg->getId());
                    function->replace(reg, m_registerInfo->getRegister(rr), false);
                }
                if(change) break;
            }
        } while(change);
    }
    return false;
}

void RegallocBase::processSpills(MIR::Function* function) {
    for(uint32_t spill : function->getRegisterInfo().getSpills()) {
        m_spiller.spill(m_registerInfo->getRegister(spill), function);
    }
    function->getRegisterInfo().getSpills().clear();
}

}