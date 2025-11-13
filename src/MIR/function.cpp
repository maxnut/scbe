#include "MIR/function.hpp"
#include "IR/function.hpp"
#include "MIR/operand.hpp"
#include "MIR/instruction.hpp"
#include "target/register_info.hpp"
#include "unit.hpp"

namespace scbe::MIR {

Function::Function(const std::string& name, IR::Function* irFunction, Target::RegisterInfo* registerInfo) : m_targetRegisterInfo(registerInfo), m_name(name), m_irFunction(irFunction) {
    for(auto& arg : irFunction->getArguments()) {
        if(arg->hasFlag(IR::Value::Flag::ByVal)) {
            m_args.push_back(nullptr);
            continue;
        }
        auto mirArg = m_targetRegisterInfo->getRegister(m_registerInfo.getNextVirtualRegister(arg->getType(), registerInfo->getClassFromType(arg->getType())));
        m_args.push_back(std::move(mirArg));
    }
}

size_t Function::getInstructionIdx(Instruction* instruction) const {
    size_t cur = 0;
    for(auto& block : m_blocks) {
        if(!block->hasInstruction(instruction)) {
            cur += block->getInstructions().size();
            continue;
        }
        cur += block->getInstructionIdx(instruction);
        break;
    }
    return cur;
}

void Function::replace(Operand* replace, Operand* with, bool copyFlags) {
    for(auto& block : m_blocks) {
        for(auto& instr : block->getInstructions()) {
            for(auto& op : instr->getOperands()) {
                if(!op || !op->equals(replace, copyFlags)) continue;

                if(copyFlags && op->getFlags() != 0) {
                    op = cloneOpWithFlags(with, op->getFlags());
                    continue;
                }

                op = with;
            }
        }
    }
}

bool Function::hasLiveIn(uint32_t reg) const {
    for(uint32_t liveIn : m_liveIns) {
        if(m_targetRegisterInfo->isSameRegister(liveIn, reg)) return true;
    }
    return false;
}

Operand* Function::cloneOpWithFlags(Operand* operand, int64_t flags) {
    switch(operand->getKind()) {
        case Operand::Kind::Register: {
            return m_targetRegisterInfo->getRegister(cast<Register>(operand)->getId(), flags);
        }
        case Operand::Kind::ImmediateInt: {
            auto imm = cast<ImmediateInt>(operand);
            return m_irFunction->getUnit()->getContext()->getImmediateInt(imm->getValue(), imm->getSize(), flags);
        }
        case Operand::Kind::ExternalSymbol: {
            auto ext = std::unique_ptr<ExternalSymbol>(new ExternalSymbol(cast<ExternalSymbol>(operand)->getName()));
            ext->setFlags(flags);
            auto ret = ext.get();
            m_irFunction->getUnit()->addSymbol(std::move(ext));
            return ret;
        }
        case Operand::Kind::GlobalAddress: {
            auto global = std::unique_ptr<GlobalAddress>(new GlobalAddress(cast<GlobalAddress>(operand)->getValue()));
            global->setFlags(flags);
            auto ret = global.get();
            m_irFunction->getUnit()->addSymbol(std::move(global));
            return ret;
        }
        default: break;
    }
    throw std::runtime_error("Unsupported operand kind");
}

}