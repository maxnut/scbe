#include "IR/folder.hpp"
#include "IR/instruction.hpp"
#include "IR/value.hpp"
#include "cast.hpp"
#include "type.hpp"
#include <sys/types.h>

namespace scbe::IR {
    
Value* Folder::foldBinOp(Instruction::Opcode opcode, Value* lhs, Value* rhs) {
    if(!lhs->isConstant() || !rhs->isConstant())
        return nullptr;

    if(lhs->isConstantInt() && rhs->isConstantInt()) {
        uint8_t bits = cast<IntegerType>(cast<ConstantInt>(lhs)->getType())->getBits();
        auto v = foldBinOpInternal<ConstantInt, int64_t>(opcode, cast<ConstantInt>(lhs), cast<ConstantInt>(rhs), [&](int64_t v) { return ConstantInt::get(bits, v, m_context); });
        if(v) return v;
        return foldBinOpInternalInt(opcode, cast<ConstantInt>(lhs), cast<ConstantInt>(rhs));
    }
    else if(lhs->isConstantFloat() && rhs->isConstantFloat()) {
        auto bits = cast<FloatType>(cast<ConstantFloat>(lhs)->getType())->getBits();
        return foldBinOpInternal<ConstantFloat, double>(opcode, cast<ConstantFloat>(lhs), cast<ConstantFloat>(rhs), [&](double v) { return ConstantFloat::get(bits, v, m_context); });
    }
    
    return nullptr;
}

Value* Folder::foldCast(Instruction::Opcode opcode, Value* value, Type* type) {
    if(value->getType() == type) return value;
    
    if(!value->isConstant())
        return nullptr;
    
    if(value->isConstantInt())
        return foldCastInternal(opcode, cast<ConstantInt>(value), type);
    else if(value->isConstantFloat())
        return foldCastInternal(opcode, cast<ConstantFloat>(value), type);

    return nullptr;
}

template<typename T, typename F>
Value* Folder::foldBinOpInternal(Instruction::Opcode opcode, T* lhs, T* rhs, std::function<T*(F)> constructor) {
    switch (opcode) {
        case Instruction::Opcode::Add: {
            return constructor(lhs->getValue() + rhs->getValue());
        }
        case Instruction::Opcode::Sub: {
            return constructor(lhs->getValue() - rhs->getValue());
        }
        case Instruction::Opcode::FMul:
        case Instruction::Opcode::UMul:
        case Instruction::Opcode::IMul: {
            return constructor(lhs->getValue() * rhs->getValue());
        }
        case Instruction::Opcode::FDiv:
        case Instruction::Opcode::UDiv:
        case Instruction::Opcode::IDiv: {
            return constructor(lhs->getValue() / rhs->getValue());
        }
        case Instruction::Opcode::ICmpEq: {
            return ConstantInt::get(1, lhs->getValue() == rhs->getValue(), m_context);
        }
        case Instruction::Opcode::ICmpNe: {
            return ConstantInt::get(1, lhs->getValue() != rhs->getValue(), m_context);
        }
        case Instruction::Opcode::ICmpGt: {
            return ConstantInt::get(1, lhs->getValue() > rhs->getValue(), m_context);
        }
        case Instruction::Opcode::UCmpGt: {
            return ConstantInt::get(1, lhs->getValue() > rhs->getValue(), m_context);
        }
        case Instruction::Opcode::ICmpGe: {
            return ConstantInt::get(1, lhs->getValue() >= rhs->getValue(), m_context);
        }
        case Instruction::Opcode::UCmpGe: {
            return ConstantInt::get(1, lhs->getValue() >= rhs->getValue(), m_context);
        }
        case Instruction::Opcode::ICmpLt: {
            return ConstantInt::get(1, lhs->getValue() < rhs->getValue(), m_context);
        }
        case Instruction::Opcode::UCmpLt: {
            return ConstantInt::get(1, lhs->getValue() < rhs->getValue(), m_context);
        }
        case Instruction::Opcode::ICmpLe: {
            return ConstantInt::get(1, lhs->getValue() <= rhs->getValue(), m_context);
        }
        case Instruction::Opcode::UCmpLe: {
            return ConstantInt::get(1, lhs->getValue() <= rhs->getValue(), m_context);
        }
        case Instruction::Opcode::FCmpEq: {
            return ConstantInt::get(1, lhs->getValue() == rhs->getValue(), m_context);
        }
        case Instruction::Opcode::FCmpNe: {
            return ConstantInt::get(1, lhs->getValue() != rhs->getValue(), m_context);
        }
        case Instruction::Opcode::FCmpGt: {
            return ConstantInt::get(1, lhs->getValue() > rhs->getValue(), m_context);
        }
        case Instruction::Opcode::FCmpGe: {
            return ConstantInt::get(1, lhs->getValue() >= rhs->getValue(), m_context);
        }
        case Instruction::Opcode::FCmpLt: {
            return ConstantInt::get(1, lhs->getValue() < rhs->getValue(), m_context);
        }
        case Instruction::Opcode::FCmpLe: {
            return ConstantInt::get(1, lhs->getValue() <= rhs->getValue(), m_context);
        }
        default:
            break;
    }

    return nullptr;
}

Value* Folder::foldBinOpInternalInt(Instruction::Opcode opcode, ConstantInt* lhs, ConstantInt* rhs) {
    uint8_t bits = cast<IntegerType>(cast<ConstantInt>(lhs)->getType())->getBits();
    switch (opcode) {
        case Instruction::Opcode::ShiftLeft: {
            return ConstantInt::get(bits, lhs->getValue() << rhs->getValue(), m_context);
        }
        case Instruction::Opcode::LShiftRight: {
            uint64_t left = lhs->getValue();
            uint64_t right = rhs->getValue();
            return ConstantInt::get(bits, left >> right, m_context);
        }
        case Instruction::Opcode::AShiftRight: {
            return ConstantInt::get(bits, lhs->getValue() >> rhs->getValue(), m_context);
        }
        case Instruction::Opcode::And: {
            return ConstantInt::get(bits, lhs->getValue() & rhs->getValue(), m_context);
        }
        case Instruction::Opcode::Or: {
            return ConstantInt::get(bits, lhs->getValue() | rhs->getValue(), m_context);
        }
        case Instruction::Opcode::Xor: {
            return ConstantInt::get(bits, lhs->getValue() ^ rhs->getValue(), m_context);
        }
        case Instruction::Opcode::IRem: {
            return ConstantInt::get(bits, lhs->getValue() % rhs->getValue(), m_context);
        }
        case Instruction::Opcode::URem: {
            uint64_t left = lhs->getValue();
            uint64_t right = rhs->getValue();
            return ConstantInt::get(bits, left % right, m_context);
        }
        default:
            break;
    }

    return nullptr;
}

template<typename T>
Value* Folder::foldCastInternal(Instruction::Opcode opcode, T* lhs, Type* type) {
    switch (opcode) {
        case Instruction::Opcode::Fptosi:
        case Instruction::Opcode::Fptoui: {
            return ConstantInt::get(cast<IntegerType>(type)->getBits(), lhs->getValue(), m_context);
        }
        case Instruction::Opcode::Sitofp:
        case Instruction::Opcode::Uitofp: {
            return ConstantFloat::get(cast<FloatType>(type)->getBits(), lhs->getValue(), m_context);
        }
        case Instruction::Opcode::Zext:
        case Instruction::Opcode::Sext:
        case Instruction::Opcode::Trunc:
        case Instruction::Opcode::Ptrtoint:
            return ConstantInt::get(cast<IntegerType>(type)->getBits(), lhs->getValue(), m_context);
        case Instruction::Opcode::Fptrunc:
        case Instruction::Opcode::Fpext:
            return ConstantInt::get(cast<FloatType>(type)->getBits(), lhs->getValue(), m_context);
        default:
            break;
    }
    return nullptr;
}

}