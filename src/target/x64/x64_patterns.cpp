#include "target/x64/x64_patterns.hpp"
#include "IR/global_value.hpp"
#include "IR/intrinsic.hpp"
#include "IR/value.hpp"
#include "ISel/DAG/value.hpp"
#include "MIR/instruction.hpp"
#include "MIR/operand.hpp"
#include "MIR/stack_frame.hpp"
#include "cast.hpp"
#include "codegen/dag_isel_pass.hpp"
#include "ISel/DAG/instruction.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "MIR/function.hpp"
#include "unit.hpp"
#include "type.hpp"

#include <cstdint>
#include <memory>

#define OPCODE(op) (uint32_t)Opcode::op

namespace scbe::Target::x64 {

using namespace ISel::DAG;

bool matchFunctionArguments(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitFunctionArguments(EMITTER_ARGS) {
    ISel::DAG::FunctionArgument* arg = (ISel::DAG::FunctionArgument*)node;
    return block->getParentFunction()->getArguments().at(arg->getSlot());
}

bool matchRegister(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitRegister(EMITTER_ARGS) {
    ISel::DAG::Register* reg = (ISel::DAG::Register*)node;
    uint32_t rclass = instrInfo->getRegisterInfo()->getClassFromType(reg->getType());
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    return ri->getRegister(block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(rclass));
}

bool matchFrameIndex(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitFrameIndex(EMITTER_ARGS) {
    return block->getParentFunction()->getStackFrame().getFrameIndex(((ISel::DAG::FrameIndex*)node)->getSlot());
}

bool matchRoot(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitRoot(EMITTER_ARGS) {
    return isel->getDagRootToMirNodes()[cast<ISel::DAG::Root>(node)];
}

bool matchConstantInt(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitConstantInt(EMITTER_ARGS) {
    ISel::DAG::ConstantInt* constant = cast<ISel::DAG::ConstantInt>(node);
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    return ctx->getImmediateInt(constant->getValue(), (MIR::ImmediateInt::Size)(std::max(1, cast<IntegerType>(constant->getType())->getBits() / 8)));
}

bool matchMultiValue(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitMultiValue(EMITTER_ARGS) {
    ISel::DAG::MultiValue* mv = cast<ISel::DAG::MultiValue>(node);
    auto ptr = std::make_unique<MIR::MultiValue>();
    MIR::MultiValue* mirMv = ptr.get();
    block->getParentFunction()->addMultiValue(std::move(ptr));
    for(auto& value : mv->getValues()) {
        mirMv->addValue(isel->emitOrGet(value, block));
    }
    return mirMv;
}

bool matchReturn(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 0;
}

MIR::Operand* emitReturn(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto lowering = std::make_unique<MIR::ReturnLowering>();
    block->addInstruction(std::move(lowering));
    return nullptr;
}

bool matchReturnOp(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 1;
}

MIR::Operand* emitReturnLowering(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto operand = isel->emitOrGet(i->getOperands().front(), block);
    auto lowering = std::make_unique<MIR::ReturnLowering>();
    lowering->addOperand(operand);
    block->addInstruction(std::move(lowering));
    return nullptr;
}

bool matchStoreInFrame(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().front()->getKind() == Node::NodeKind::FrameIndex;
}

MIR::Operand* emitStoreInFrame(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    Opcode op = Opcode::Count;
    auto from = isel->emitOrGet(i->getOperands().at(1), block);
    if(from->isRegister()) {
        auto rr = cast<MIR::Register>(from);
        Type* ty = cast<ISel::DAG::Value>(extractOperand(i->getOperands().at(1)))->getType();
        op = (Opcode)selectOpcode(layout, ty, {OPCODE(Mov8mr), OPCODE(Mov16mr), OPCODE(Mov32mr), OPCODE(Mov64mr)}, {OPCODE(Movssmr), OPCODE(Movsdmr)});
    }
    else if(from->isFrameIndex()) {
        auto tmp = instrInfo->getRegisterInfo()->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        auto frameIndex = cast<MIR::FrameIndex>(from);
        MIR::StackSlot slot = block->getParentFunction()->getStackFrame().getStackSlot(frameIndex->getIndex());
        xInstrInfo->stackSlotAddress(block, block->last(), slot, tmp);
        from = tmp;
        op = Opcode::Mov64mr;
    }
    else if(from->isImmediateInt()) {
        auto imm = cast<MIR::ImmediateInt>(from);
        op = (Opcode)selectOpcode(imm->getSize(), false, {OPCODE(Mov8mi), OPCODE(Mov16mi), OPCODE(Mov32mi), OPCODE(Movm64i32)}, {});
    }
    else {
        throw std::runtime_error("Unsupported operand type");
    }
    auto frameIndex = ((FrameIndex*)extractOperand(i->getOperands().at(0)));
    MIR::StackSlot slot = block->getParentFunction()->getStackFrame().getStackSlot(frameIndex->getSlot());
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    block->addInstruction(xInstrInfo->operandToMemory((uint32_t)op, from, ri->getRegister(RBP), -(int32_t)slot.m_offset));
    return nullptr;
}

bool matchStoreInPtrRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().front()));
}

MIR::Operand* emitStoreInPtrRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Opcode op = Opcode::Count;
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    auto from = isel->emitOrGet(i->getOperands().at(1), block);
    if(from->isRegister()) {
        auto rr = cast<MIR::Register>(from);
        Type* ty = cast<ISel::DAG::Value>(extractOperand(i->getOperands().at(1)))->getType();
        op = (Opcode)selectOpcode(layout, ty, {OPCODE(Mov8mr), OPCODE(Mov16mr), OPCODE(Mov32mr), OPCODE(Mov64mr)}, {OPCODE(Movssmr), OPCODE(Movsdmr)});
    }
    else if(from->isFrameIndex()) {
        auto tmp = instrInfo->getRegisterInfo()->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        auto frameIndex = cast<MIR::FrameIndex>(from);
        MIR::StackSlot slot = block->getParentFunction()->getStackFrame().getStackSlot(frameIndex->getIndex());
        xInstrInfo->stackSlotAddress(block, block->last(), slot, tmp);
        from = tmp;
        op = Opcode::Mov64mr;
    }
    else if (from->isImmediateInt()) {
        auto imm = cast<MIR::ImmediateInt>(from);
        op = (Opcode)selectOpcode(imm->getSize(), false, {OPCODE(Mov8mi), OPCODE(Mov16mi), OPCODE(Mov32mi), OPCODE(Movm64i32)}, {});
    }
    else {
        throw std::runtime_error("Unsupported operand type");
    }

    auto rr = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    block->addInstruction(xInstrInfo->operandToMemory((uint32_t)op, from, rr, 0));
    return nullptr;
}

bool matchLoadFromFrame(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().back())->getKind() == Node::NodeKind::FrameIndex;
}

MIR::Operand* emitLoadFromFrame(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto frameIndex = ((FrameIndex*)extractOperand(i->getOperands().at(0)));
    MIR::StackSlot slot = block->getParentFunction()->getStackFrame().getStackSlot(frameIndex->getSlot());
    auto result = isel->emitOrGet(extractOperand(i->getResult()), block);
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(result->isRegister()) {
        Opcode op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Mov8rm), OPCODE(Mov16rm), OPCODE(Mov32rm), OPCODE(Mov64rm)}, {OPCODE(Movssrm), OPCODE(Movsdrm)});
        block->addInstruction(xInstrInfo->memoryToOperand((uint32_t)op, result, ri->getRegister(RBP), -(int32_t)slot.m_offset));
    }
    else if(result->isMultiValue()) {
        MIR::MultiValue* multi = cast<MIR::MultiValue>(result);
        for(size_t i = 0; i < multi->getValues().size(); i++) {
            MIR::Register* value = cast<MIR::Register>(multi->getValues().at(i));
            uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(value->getId(), block->getParentFunction()->getRegisterInfo());
            size_t size = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();

            Opcode op = (Opcode)selectOpcode(size, classid == FPR, {OPCODE(Mov8rm), OPCODE(Mov16rm), OPCODE(Mov32rm), OPCODE(Mov64rm)}, {OPCODE(Movssrm), OPCODE(Movsdrm)});
            block->addInstruction(xInstrInfo->memoryToOperand((uint32_t)op, value, ri->getRegister(RBP), -(int32_t)slot.m_offset + (i * size)));
        }
    }
    return result;
}

bool matchLoadFromPtrRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().back()));
}

MIR::Operand* emitLoadFromPtrRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto result = isel->emitOrGet(extractOperand(i->getResult()), block);
    auto rr = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    if(result->isRegister()) {
        Opcode op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Mov8rm), OPCODE(Mov16rm), OPCODE(Mov32rm), OPCODE(Mov64rm)}, {OPCODE(Movssrm), OPCODE(Movsdrm)});
        block->addInstruction(xInstrInfo->memoryToOperand((uint32_t)op, result, rr, 0));
    }
    else if(result->isMultiValue()) {
        MIR::MultiValue* multi = cast<MIR::MultiValue>(result);
        for(size_t i = 0; i < multi->getValues().size(); i++) {
            MIR::Register* value = cast<MIR::Register>(multi->getValues().at(i));
            uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(value->getId(), block->getParentFunction()->getRegisterInfo());
            size_t size = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();

            Opcode op = (Opcode)selectOpcode(size, classid == FPR, {OPCODE(Mov8rm), OPCODE(Mov16rm), OPCODE(Mov32rm), OPCODE(Mov64rm)}, {OPCODE(Movssrm), OPCODE(Movsdrm)});
            block->addInstruction(xInstrInfo->memoryToOperand((uint32_t)op, value, rr, i * size));
        }
    }
    return result;
}


bool matchAddImmediates(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
        extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}

// should not happen with basic optimizations. return a register to not break other patterns
MIR::Operand* emitAddImmediates(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto c1 = (ConstantInt*)extractOperand(i->getOperands().at(0));
    auto c2 = (ConstantInt*)extractOperand(i->getOperands().at(1));
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    auto immediate = ctx->getImmediateInt(c1->getValue() + c2->getValue(), (MIR::ImmediateInt::Size)layout->getSize(c1->getType()));
    auto returnReg = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), immediate, returnReg, layout->getSize(i->getResult()->getType()), false);
    return returnReg;
}

bool matchAddRegisters(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        isRegister(extractOperand(i->getOperands().at(0))) &&
        isRegister(extractOperand(i->getOperands().at(1)));
}

MIR::Operand* emitAddRegisters(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto left = isel->emitOrGet(i->getOperands().at(0), block);
    auto resultRegister = isel->emitOrGet(extractOperand(i->getResult()), block);
    Opcode op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Mov8rr), OPCODE(Mov16rr), OPCODE(Mov32rr), OPCODE(Mov64rr)}, {OPCODE(Movssrr), OPCODE(Movsdrr)});
    block->addInstruction(instr((uint32_t)op, resultRegister, left));
    op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Add8rr), OPCODE(Add16rr), OPCODE(Add32rr), OPCODE(Add64rr)}, {OPCODE(Addssrr), OPCODE(Addsdrr)});
    block->addInstruction(instr((uint32_t)op, resultRegister, isel->emitOrGet(i->getOperands().at(1), block)));
    return resultRegister;
}

bool matchAddRegistersLea(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(
        cast<ISel::DAG::Value>(extractOperand(i->getResult()))->getType()
    );
    return i->getOperands().size() == 2 &&
        isRegister(extractOperand(i->getOperands().at(0))) &&
        isRegister(extractOperand(i->getOperands().at(1))) && size > 1;
}

MIR::Operand* emitAddRegistersLea(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto left = isel->emitOrGet(i->getOperands().at(0), block);
    auto right = isel->emitOrGet(i->getOperands().at(1), block);
    auto resultRegister = cast<MIR::Register>(isel->emitOrGet(extractOperand(i->getResult()), block));
    size_t size = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(resultRegister->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    if(size != 8) {
        left = block->getParentFunction()->cloneOpWithFlags(left, Force64BitRegister);
        right = block->getParentFunction()->cloneOpWithFlags(right, Force64BitRegister);
    }
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    uint32_t op = selectOpcode(size, false, {0, OPCODE(Lea16rm), OPCODE(Lea32rm), OPCODE(Lea64rm)}, {});
    block->addInstruction(
        xInstrInfo->memoryToOperand(op, resultRegister, cast<MIR::Register>(left), 0, cast<MIR::Register>(right))
    );
    return resultRegister;
}

bool matchAddRegisterImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        (isRegister(extractOperand(i->getOperands().at(0))) &&
        extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt) ||
        (extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
        isRegister(extractOperand(i->getOperands().at(1))));
}

MIR::Operand* emitAddRegisterImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    bool swap = extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt;
    auto left = isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block);
    auto right = isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block);
    auto resultRegister = isel->emitOrGet(extractOperand(i->getResult()), block);

    Opcode op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Mov8rr), OPCODE(Mov16rr), OPCODE(Mov32rr), OPCODE(Mov64rr)}, {OPCODE(Movssrr), OPCODE(Movsdrr)});
    block->addInstruction(instr((uint32_t)op, resultRegister, left));
    op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Add8ri), OPCODE(Add16ri), OPCODE(Add32ri), OPCODE(Add64r32i)}, {});
    if((uint32_t)op == 0) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        block->addInstruction(instr(OPCODE(Movr64i64), tmp, right));
        right = tmp;
        op = Opcode::Add64rr;
    }
    block->addInstruction(instr((uint32_t)op, resultRegister, right));
    return resultRegister;
}

bool matchSubImmediates(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
        extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}

MIR::Operand* emitSubImmediates(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto c1 = (ConstantInt*)extractOperand(i->getOperands().at(0));
    auto c2 = (ConstantInt*)extractOperand(i->getOperands().at(1));
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    auto immediate = ctx->getImmediateInt(c1->getValue() - c2->getValue(), (MIR::ImmediateInt::Size)layout->getSize(c1->getType()));
    auto returnReg = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), immediate, returnReg, layout->getSize(i->getResult()->getType()), false);
    return returnReg;
}

bool matchSubRegisters(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        isRegister(extractOperand(i->getOperands().at(0))) &&
        isRegister(extractOperand(i->getOperands().at(1)));
}

MIR::Operand* emitSubRegisters(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto left = isel->emitOrGet(i->getOperands().at(0), block);
    auto resultRegister = isel->emitOrGet(extractOperand(i->getResult()), block);
    Opcode op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Mov8rr), OPCODE(Mov16rr), OPCODE(Mov32rr), OPCODE(Mov64rr)}, {OPCODE(Movssrr), OPCODE(Movsdrr)});
    block->addInstruction(instr((uint32_t)op, resultRegister, left));
    op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Sub8rr), OPCODE(Sub16rr), OPCODE(Sub32rr), OPCODE(Sub64rr)}, {OPCODE(Subssrr), OPCODE(Subsdrr)});
    block->addInstruction(instr((uint32_t)op, resultRegister, isel->emitOrGet(i->getOperands().at(1), block)));
    return resultRegister;
}

bool matchSubRegisterImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        ((isRegister(extractOperand(i->getOperands().at(0))) &&
        extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt)
            ||
        (extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
        isRegister(extractOperand(i->getOperands().at(1)))));
}

MIR::Operand* emitSubRegisterImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto left = isel->emitOrGet(i->getOperands().at(0), block);
    auto right = isel->emitOrGet(i->getOperands().at(1), block);
    auto resultRegister = isel->emitOrGet(extractOperand(i->getResult()), block);
    instrInfo->move(block, block->last(), left, resultRegister, layout->getSize(i->getResult()->getType()), false);
    Opcode op;

    if(right->isImmediateInt()) {
        op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Sub8ri), OPCODE(Sub16ri), OPCODE(Sub32ri), OPCODE(Sub64r32i)}, {});
    }
    else {
        op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Sub8rr), OPCODE(Sub16rr), OPCODE(Sub32rr), OPCODE(Sub64rr)}, {});
    }

    if((uint32_t)op == 0) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        block->addInstruction(instr(OPCODE(Movr64i64), tmp, right));
        right = tmp;
        op = Opcode::Sub64rr;
    }
    block->addInstruction(instr((uint32_t)op, resultRegister, right));
    return resultRegister;
}

bool matchPhi(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() > 0;
}

MIR::Operand* emitPhi(EMITTER_ARGS) {
    auto* phi = cast<ISel::DAG::Instruction>(node);

    auto* dest = cast<MIR::Register>(isel->emitOrGet(extractOperand(phi->getResult()), block));
    assert(dest->isRegister());

    for (size_t idx = 0; idx < phi->getOperands().size(); idx += 2) {
        auto* valueNode = phi->getOperands().at(idx);
        auto* predBlock = cast<MIR::Block>(
            isel->emitOrGet(phi->getOperands().at(idx + 1), block)
        );
        auto rightBlock = cast<MIR::Block>(isel->emitOrGet(valueNode->getRoot(), block));
        assert(phi->getOperands().at(idx + 1) != phi->getRoot());

        auto src = isel->emitOrGet(valueNode, rightBlock);
        predBlock->addPhiLowering(dest, src);
    }

    return dest;
}


bool matchJump(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() == 1;
}

MIR::Operand* emitJump(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(extractOperand(i->getOperands().at(0)), block)));
    return nullptr;
}

bool matchCondJumpImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() > 1 && extractOperand(i->getOperands().at(2))->getKind() == Node::NodeKind::ConstantInt;
}

MIR::Operand* emitCondJumpImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::ImmediateInt* cond = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(2), block));
    assert(cond->getValue() == 0 || cond->getValue() == 1);
    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(i->getOperands().at(cond->getValue()), block)));
    return nullptr;
}

bool matchCondJumpRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() > 1 && isRegister(extractOperand(i->getOperands().at(2)));
}

MIR::Operand* emitCondJumpRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* cond = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(2), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(cond->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    Opcode testOp = (Opcode)selectOpcode(fromSize, false, {OPCODE(Test8rr), OPCODE(Test16rr), OPCODE(Test32rr), OPCODE(Test64rr)}, {});
    block->addInstruction(instr((uint32_t)testOp, cond, cond));
    block->addInstruction(instr(OPCODE(Jne), isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(i->getOperands().at(1), block)));
    return nullptr;
}

bool matchCondJumpComparisonRI(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() > 2 && i->getOperands().at(2)->isCmp() &&
        ((cast<Instruction>(i->getOperands().at(2))->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt &&
        isRegister(extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(1))))
        ||
        (isRegister(extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(0))) &&
        cast<Instruction>(i->getOperands().at(2))->getOperands().at(1)->getKind() == Node::NodeKind::ConstantInt));
}

MIR::Operand* emitCondJumpComparisonRI(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Instruction* comparison = (Instruction*)i->getOperands().at(2);
    bool swap = comparison->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt;

    ConstantInt* rhs = (ConstantInt*)extractOperand(comparison->getOperands().at(swap ? 0 : 1));
    size_t size = layout->getSize(rhs->getType());
    auto right = isel->emitOrGet(comparison->getOperands().at(swap ? 0 : 1), block);
    Opcode cmpOp = (Opcode)selectOpcode(layout, rhs->getType(), {OPCODE(Cmp8ri), OPCODE(Cmp16ri), OPCODE(Cmp32ri), 0}, {});
    if((uint32_t)cmpOp == 0) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        cmpOp = immSizeFromValue(cast<MIR::ImmediateInt>(right)->getValue()) > 4 ? Opcode::Movr64i64 : Opcode::Movr64i32;
        block->addInstruction(instr((uint32_t)cmpOp, tmp, right));
        right = tmp;
        cmpOp = Opcode::Cmp64rr;
    }

    MIR::Register* rr = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(swap ? 1 : 0), block));

    block->addInstruction(instr((uint32_t)cmpOp, rr, right));

    Opcode jmpOp;
    switch(comparison->getKind()) {
        case Node::NodeKind::ICmpEq:
            jmpOp = Opcode::Je;
            break;
        case Node::NodeKind::ICmpNe:
            jmpOp = Opcode::Jne;
            break;
        case Node::NodeKind::ICmpGt:
            jmpOp = Opcode::Jg;
            break;
        case Node::NodeKind::UCmpGt:
            jmpOp = Opcode::Ja;
            break;
        case Node::NodeKind::ICmpGe:
            jmpOp = Opcode::Jge;
            break;
        case Node::NodeKind::UCmpGe:
            jmpOp = Opcode::Jae;
            break;
        case Node::NodeKind::ICmpLt:
            jmpOp = Opcode::Jl;
            break;
        case Node::NodeKind::UCmpLt:
            jmpOp = Opcode::Jb;
            break;
        case Node::NodeKind::ICmpLe:
            jmpOp = Opcode::Jle;
            break;
        case Node::NodeKind::UCmpLe:
            jmpOp = Opcode::Jbe;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)jmpOp, isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(i->getOperands().at(1), block)));
    return nullptr;
}

bool matchCondJumpComparisonII(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() > 2 && i->getOperands().at(2)->isCmp()&&
        cast<Instruction>(i->getOperands().at(2))->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt &&
        cast<Instruction>(i->getOperands().at(2))->getOperands().at(1)->getKind() == Node::NodeKind::ConstantInt;
}

MIR::Operand* emitCondJumpComparisonII(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Instruction* comparison = (Instruction*)i->getOperands().at(2);
    ConstantInt* lhs = (ConstantInt*)extractOperand(comparison->getOperands().at(0));
    ConstantInt* rhs = (ConstantInt*)extractOperand(comparison->getOperands().at(1));
    bool isTrue = false;
    switch(comparison->getKind()) {
        case Node::NodeKind::ICmpEq:
            isTrue = lhs->getValue() == rhs->getValue();
            break;
        case Node::NodeKind::ICmpNe:
            isTrue = lhs->getValue() != rhs->getValue();
            break;
        case Node::NodeKind::ICmpGt:
        case Node::NodeKind::UCmpGt:
            isTrue = lhs->getValue() > rhs->getValue();
            break;
        case Node::NodeKind::ICmpGe:
        case Node::NodeKind::UCmpGe:
            isTrue = lhs->getValue() >= rhs->getValue();
            break;
        case Node::NodeKind::ICmpLt:
        case Node::NodeKind::UCmpLt:
            isTrue = lhs->getValue() < rhs->getValue();
            break;
        case Node::NodeKind::ICmpLe:
        case Node::NodeKind::UCmpLe:
            isTrue = lhs->getValue() <= rhs->getValue();
            break;
        default:
            break;
    }

    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(i->getOperands().at(isTrue ? 0 : 1), block)));
    return nullptr;
}

bool matchCondJumpComparisonRR(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    
    if(i->getKind() != Node::NodeKind::Jump || i->getOperands().size() <= 2 || !i->getOperands().at(2)->isCmp()) return false;
    
    auto left = extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(0));
    auto right = extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(1));
    return isRegister(left) && cast<Value>(left)->getType()->isIntType() &&
        isRegister(right) && cast<Value>(right)->getType()->isIntType();
}

MIR::Operand* emitCondJumpComparisonRR(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Instruction* comparison = (Instruction*)i->getOperands().at(2);

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(0), block));
    MIR::Register* rhs = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(1), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(lhs->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    Opcode cmpOp = (Opcode)selectOpcode(fromSize, false, {OPCODE(Cmp8rr), OPCODE(Cmp16rr), OPCODE(Cmp32rr), OPCODE(Cmp64rr)}, {});

    block->addInstruction(instr((uint32_t)cmpOp, lhs, rhs));

    Opcode jmpOp;
    switch(comparison->getKind()) {
        case Node::NodeKind::ICmpEq:
            jmpOp = Opcode::Je;
            break;
        case Node::NodeKind::ICmpNe:
            jmpOp = Opcode::Jne;
            break;
        case Node::NodeKind::ICmpGt:
            jmpOp = Opcode::Jg;
            break;
        case Node::NodeKind::UCmpGt:
            jmpOp = Opcode::Ja;
            break;
        case Node::NodeKind::ICmpGe:
            jmpOp = Opcode::Jge;
            break;
        case Node::NodeKind::UCmpGe:
            jmpOp = Opcode::Jae;
            break;
        case Node::NodeKind::ICmpLt:
            jmpOp = Opcode::Jl;
            break;
        case Node::NodeKind::UCmpLt:
            jmpOp = Opcode::Jb;
            break;
        case Node::NodeKind::ICmpLe:
            jmpOp = Opcode::Jle;
            break;
        case Node::NodeKind::UCmpLe:
            jmpOp = Opcode::Jbe;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)jmpOp, isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(i->getOperands().at(1), block)));
    return nullptr;
}

bool matchFCondJumpComparisonRF(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() > 2 && i->getOperands().at(2)->isCmp() &&
        ((cast<Instruction>(i->getOperands().at(2))->getOperands().at(0)->getKind() == Node::NodeKind::LoadConstant &&
        isRegister(extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(1))))
        ||
        (isRegister(extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(0))) &&
        cast<Instruction>(i->getOperands().at(2))->getOperands().at(1)->getKind() == Node::NodeKind::LoadConstant));
}

bool matchFCondJumpComparisonRR(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    
    if(i->getKind() != Node::NodeKind::Jump || i->getOperands().size() <= 2 || !i->getOperands().at(2)->isCmp()) return false;
    
    auto left = extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(0));
    auto right = extractOperand(cast<Instruction>(i->getOperands().at(2))->getOperands().at(1));
    return isRegister(left) && cast<Value>(left)->getType()->isFltType() &&
        isRegister(right) && cast<Value>(right)->getType()->isFltType();
}

MIR::Operand* emitFCondJumpComparisonRR(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Instruction* comparison = (Instruction*)i->getOperands().at(2);

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(0), block));
    MIR::Register* rhs = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(1), block));
    
    FloatType* fromType = (FloatType*)(cast<Value>(extractOperand(comparison->getOperands().at(0)))->getType());

    Opcode cmpOp = (Opcode)selectOpcode(layout, fromType, {}, {OPCODE(Ucomissrr), OPCODE(Ucomisdrr)});

    block->addInstruction(instr((uint32_t)cmpOp, lhs, rhs));

    Opcode jmpOp;
    switch(comparison->getKind()) {
        case Node::NodeKind::FCmpEq:
            jmpOp = Opcode::Je;
            break;
        case Node::NodeKind::FCmpNe:
            jmpOp = Opcode::Jne;
            break;
        case Node::NodeKind::FCmpGt:
            jmpOp = Opcode::Ja;
            break;
        case Node::NodeKind::FCmpGe:
            jmpOp = Opcode::Jae;
            break;
        case Node::NodeKind::FCmpLt:
            jmpOp = Opcode::Jb;
            break;
        case Node::NodeKind::FCmpLe:
            jmpOp = Opcode::Jbe;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)jmpOp, isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(Jmp), isel->emitOrGet(i->getOperands().at(1), block)));
    return nullptr;
}



bool matchCmpRegisterImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->isCmp() &&
        ((i->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt && isRegister(extractOperand(i->getOperands().at(1))))
        ||
        (isRegister(extractOperand(i->getOperands().at(0))) && i->getOperands().at(1)->getKind() == Node::NodeKind::ConstantInt));
}
MIR::Operand* emitCmpRegisterImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    bool swap = i->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt;

    auto left = isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block);
    auto right = isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block);
    auto ret = isel->emitOrGet(i->getResult(), block);

    ConstantInt* rhs = (ConstantInt*)extractOperand(i->getOperands().at(swap ? 0 : 1));
    size_t size = layout->getSize(rhs->getType());
    Opcode cmpOp = (Opcode)selectOpcode(layout, rhs->getType(), {OPCODE(Cmp8ri), OPCODE(Cmp16ri), OPCODE(Cmp32ri), 0}, {});
    if((uint32_t)cmpOp == 0) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        cmpOp = immSizeFromValue(cast<MIR::ImmediateInt>(right)->getValue()) > 4 ? Opcode::Movr64i64 : Opcode::Movr64i32;
        block->addInstruction(instr((uint32_t)cmpOp, tmp, right));
        right = tmp;
        cmpOp = Opcode::Cmp64rr;
    }

    block->addInstruction(instr((uint32_t)cmpOp, left, right));

    Opcode setOp;
    switch(i->getKind()) {
        case Node::NodeKind::ICmpEq:
            setOp = Opcode::Sete;
            break;
        case Node::NodeKind::ICmpNe:
            setOp = Opcode::Setne;
            break;
        case Node::NodeKind::ICmpGt:
            setOp = Opcode::Setg;
            break;
        case Node::NodeKind::UCmpGt:
            setOp = Opcode::Seta;
            break;
        case Node::NodeKind::ICmpGe:
            setOp = Opcode::Setge;
            break;
        case Node::NodeKind::UCmpGe:
            setOp = Opcode::Setae;
            break;
        case Node::NodeKind::ICmpLt:
            setOp = Opcode::Setl;
            break;
        case Node::NodeKind::UCmpLt:
            setOp = Opcode::Setb;
            break;
        case Node::NodeKind::ICmpLe:
            setOp = Opcode::Setle;
            break;
        case Node::NodeKind::UCmpLe:
            setOp = Opcode::Setbe;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)setOp, ret));
    return ret;
}

bool matchCmpRegisterRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->isCmp() && isRegister(extractOperand(i->getOperands().at(0))) && isRegister(extractOperand(i->getOperands().at(1)));
}

MIR::Operand* emitCmpRegisterRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* rhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(lhs->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    Opcode cmpOp = (Opcode)selectOpcode(fromSize, false, {OPCODE(Cmp8rr), OPCODE(Cmp16rr), OPCODE(Cmp32rr), OPCODE(Cmp64rr)}, {});

    block->addInstruction(instr((uint32_t)cmpOp, lhs, rhs));

    Opcode setOp;
    switch(i->getKind()) {
        case Node::NodeKind::ICmpEq:
            setOp = Opcode::Sete;
            break;
        case Node::NodeKind::ICmpNe:
            setOp = Opcode::Setne;
            break;
        case Node::NodeKind::ICmpGt:
            setOp = Opcode::Setg;
            break;
        case Node::NodeKind::UCmpGt:
            setOp = Opcode::Seta;
            break;
        case Node::NodeKind::ICmpGe:
            setOp = Opcode::Setge;
            break;
        case Node::NodeKind::UCmpGe:
            setOp = Opcode::Setae;
            break;
        case Node::NodeKind::ICmpLt:
            setOp = Opcode::Setl;
            break;
        case Node::NodeKind::UCmpLt:
            setOp = Opcode::Setb;
            break;
        case Node::NodeKind::ICmpLe:
            setOp = Opcode::Setle;
            break;
        case Node::NodeKind::UCmpLe:
            setOp = Opcode::Setbe;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)setOp, ret));
    return ret;
}

bool matchCmpImmediateImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->isCmp() && extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}

MIR::Operand* emitCmpImmediateImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);

    MIR::ImmediateInt* lhs = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::ImmediateInt* rhs = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));

    int64_t val1 = lhs->getValue();
    int64_t val2 = rhs->getValue();
    int64_t cmpRes = 0;

    switch(i->getKind()) {
        case Node::NodeKind::ICmpEq:
            cmpRes = val1 == val2;
            break;
        case Node::NodeKind::ICmpNe:
            cmpRes = val1 != val2;
            break;
        case Node::NodeKind::ICmpGt:
            cmpRes = val1 > val2;
            break;
        case Node::NodeKind::UCmpGt:
            cmpRes = (uint64_t)val1 > (uint64_t)val2;
            break;
        case Node::NodeKind::ICmpGe:
            cmpRes = val1 >= val2;
            break;
        case Node::NodeKind::UCmpGe:
            cmpRes = (uint64_t)val1 >= (uint64_t)val2;
            break;
        case Node::NodeKind::ICmpLt:
            cmpRes = val1 < val2;
            break;
        case Node::NodeKind::UCmpLt:
            cmpRes = (uint64_t)val1 < (uint64_t)val2;
            break;
        case Node::NodeKind::ICmpLe:
            cmpRes = val1 <= val2;
            break;
        case Node::NodeKind::UCmpLe:
            cmpRes = (uint64_t)val1 <= (uint64_t)val2;
            break;
        default:
            break;
    }

    size_t size = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(ret->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    instrInfo->move(block, block->last(), context->getImmediateInt(cmpRes, MIR::ImmediateInt::imm8), ret, size, false);

    return ret;
}

bool matchFCmpRegisterFloat(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->isCmp() &&
        ((i->getOperands().at(0)->getKind() == Node::NodeKind::LoadConstant && isRegister(extractOperand(i->getOperands().at(1)))) ||
        (isRegister(extractOperand(i->getOperands().at(0))) && i->getOperands().at(1)->getKind() == Node::NodeKind::LoadConstant));
}

bool matchFCmpRegisterRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    
    if(i->isCmp()) return false;
    
    auto left = i->getOperands().at(0);
    auto right = i->getOperands().at(1);
    return isRegister(left) && cast<Value>(extractOperand(left))->getType()->isFltType() &&
        isRegister(right) && cast<Value>(extractOperand(right))->getType()->isFltType();
}
MIR::Operand* emitFCmpRegisterRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* rhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    
    FloatType* fromType = (FloatType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());

    Opcode cmpOp = (Opcode)selectOpcode(layout, fromType, {}, {OPCODE(Ucomissrr), OPCODE(Ucomisdrr)});

    block->addInstruction(instr((uint32_t)cmpOp, lhs, rhs));

    Opcode setOp;
    switch(i->getKind()) {
        case Node::NodeKind::FCmpEq:
            setOp = Opcode::Sete;
            break;
        case Node::NodeKind::FCmpNe:
            setOp = Opcode::Setne;
            break;
        case Node::NodeKind::FCmpGt:
            setOp = Opcode::Seta;
            break;
        case Node::NodeKind::FCmpGe:
            setOp = Opcode::Setae;
            break;
        case Node::NodeKind::FCmpLt:
            setOp = Opcode::Setb;
            break;
        case Node::NodeKind::FCmpLe:
            setOp = Opcode::Setbe;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)setOp, ret));
    return ret;
}



bool matchConstantFloat(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().at(0)->getKind() == Node::NodeKind::ConstantFloat;
}

MIR::Operand* emitConstantFloat(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    ConstantFloat* constant = cast<ConstantFloat>(i->getOperands().at(0));
    FloatType* ftype = (FloatType*)constant->getType();
    IR::ConstantFloat* cnst = IR::ConstantFloat::get(ftype->getBits(), constant->getValue(), context);
    Unit* unit = block->getParentFunction()->getIRFunction()->getUnit();
    MIR::Operand* rr = isel->emitOrGet(i->getResult(), block);
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    uint32_t op = ftype->getBits() == 32 ? OPCODE(Movssrm) : OPCODE(Movsdrm);
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    block->addInstruction(xInstrInfo->memoryToOperand(op, rr, ri->getRegister(RIP), 0, nullptr, 1,
        IR::GlobalVariable::get(*unit, cnst->getType(), cnst, IR::Linkage::Internal)->getMachineGlobalAddress(*unit)));
    return rr;
}

bool matchGEP(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitGEP(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Operand* base = isel->emitOrGet(i->getOperands().at(0), block);
    Type* curType = ((Value*)extractOperand(i->getOperands().at(0), false))->getType();
    int64_t curOff = 0;

    if(base->getKind() == MIR::Operand::Kind::FrameIndex) {
        auto fi = cast<MIR::FrameIndex>(base);
        auto frame = block->getParentFunction()->getStackFrame().getStackSlot(fi->getIndex());
        curOff = -frame.m_offset;
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        base = ri->getRegister(RBP);
    }

    for(size_t idx = 1; idx < i->getOperands().size(); idx++) {
        MIR::Operand* index = isel->emitOrGet(i->getOperands().at(idx), block);
        if(index->isImmediateInt()) {
            MIR::ImmediateInt* imm = (MIR::ImmediateInt*)index;
            for(size_t i = 0; i < imm->getValue(); i++) {
                Type* ty = curType->isStructType() ? curType->getContainedTypes().at(i) : curType->getContainedTypes().at(0);
                curOff += layout->getSize(ty);
            }
            curType = curType->isStructType() ? curType->getContainedTypes().at(imm->getValue()) : curType->getContainedTypes().at(0);
            continue;
        }
        else {
            x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
            curOff = 0;
            Type* ty = curType->getContainedTypes().at(0);
            block->addInstruction(xInstrInfo->memoryToOperand(OPCODE(Lea64rm), base, cast<MIR::Register>(base), 0, cast<MIR::Register>(index), layout->getSize(ty), nullptr));
        }
    }

    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    if(curOff == 0) return base;
    block->addInstruction(xInstrInfo->memoryToOperand(OPCODE(Lea64rm), ret, cast<MIR::Register>(base), curOff));
    return ret;
}

bool matchSwitch(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitSwitchLowering(EMITTER_ARGS) {
    ISel::DAG::Instruction* in = cast<ISel::DAG::Instruction>(node);
    auto swRet = std::make_unique<MIR::SwitchLowering>();
    swRet->addOperand(isel->emitOrGet(in->getOperands().at(0), block));
    swRet->addOperand(isel->emitOrGet(in->getOperands().at(1), block));
    for(size_t i = 2; i < in->getOperands().size(); i += 2) {
        swRet->addOperand(isel->emitOrGet(in->getOperands().at(i), block));
        swRet->addOperand(isel->emitOrGet(in->getOperands().at(i + 1), block));
    }
    block->addInstruction(std::move(swRet));
    return nullptr;
}

bool matchCall(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitCallLowering(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Call* call = cast<Call>(i);
    Node* callee = i->getOperands().at(0);

    MIR::Operand* ret = !call->isResultUsed() || call->getResult()->getType()->isVoidType() ? nullptr : isel->emitOrGet(i->getResult(), block);
    std::unique_ptr<MIR::CallLowering> ins = std::make_unique<MIR::CallLowering>();
    ins->addOperand(ret);
    if(i->getResult()) ins->addType(i->getResult()->getType());
    else ins->addType(context->getVoidType());

    if(callee->getKind() == Node::NodeKind::LoadGlobal) {
        ISel::DAG::Function* func = cast<ISel::DAG::Function>(cast<Instruction>(callee)->getOperands().at(0));
        IR::Function* irFunc = func->getFunction();
            Unit* unit = block->getParentFunction()->getIRFunction()->getUnit();
        if(!irFunc->hasBody()) {
            ins->addOperand(unit->getOrInsertExternal(irFunc->getName()));
        }
        else {
            ins->addOperand(irFunc->getMachineGlobalAddress(*unit));
        }
    }
    else {
        ins->addOperand(isel->emitOrGet(callee, block));
    }

    for(size_t idx = 1; idx < call->getOperands().size(); idx++) {
        ins->addOperand(isel->emitOrGet(i->getOperands().at(idx), block));
        auto op = cast<ISel::DAG::Value>(extractOperand(i->getOperands().at(idx)));
        ins->addType(op->getType());
    }
    block->addInstruction(std::move(ins));
    return ret;
}

bool matchIntrinsicCall(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Call* call = cast<Call>(i);
    if(call->getOperands().at(0)->getKind() == Node::NodeKind::GlobalValue) {
        return cast<Function>(call->getOperands().at(0))->getFunction()->isIntrinsic();
    }
    else if(call->getOperands().at(0)->getKind() == Node::NodeKind::LoadGlobal) {
        auto loadGlobal = cast<Instruction>(call->getOperands().at(0));
        return cast<Function>(loadGlobal->getOperands().at(0))->getFunction()->isIntrinsic();
    }
    return false;
}

MIR::Operand* emitIntrinsicCall(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Call* call = cast<Call>(i);
    IR::IntrinsicFunction* intrinsic = nullptr;
    if(call->getOperands().at(0)->getKind() == Node::NodeKind::GlobalValue) {
        intrinsic = cast<IR::IntrinsicFunction>(cast<Function>(call->getOperands().at(0))->getFunction());
    }
    else if(call->getOperands().at(0)->getKind() == Node::NodeKind::LoadGlobal) {
        auto loadGlobal = cast<Instruction>(call->getOperands().at(0));
        intrinsic = cast<IR::IntrinsicFunction>(cast<Function>(loadGlobal->getOperands().at(0))->getFunction());
    }
    MIR::Operand* ret = i->getResult()->getType()->isVoidType() ? nullptr : isel->emitOrGet(i->getResult(), block);

    switch (intrinsic->getIntrinsicName()) {
        case IR::IntrinsicFunction::Memcpy: {
            MIR::Operand* dst = isel->emitOrGet(i->getOperands().at(1), block);
            MIR::Operand* src = isel->emitOrGet(i->getOperands().at(2), block);
            MIR::Operand* len = isel->emitOrGet(i->getOperands().at(3), block);
            x64InstructionInfo* x64Info = (x64InstructionInfo*)instrInfo;
            MIR::StackFrame& frame = block->getParentFunction()->getStackFrame();
            RegisterInfo* ri = instrInfo->getRegisterInfo();
            // assume size 8 and no float since we should always have pointers
            if(dst->isFrameIndex())
                x64Info->stackSlotAddress(block, block->getInstructions().size(), frame.getStackSlot(cast<MIR::FrameIndex>(dst)->getIndex()), ri->getRegister(RDI));
            else
                instrInfo->move(block, block->getInstructions().size(), dst, ri->getRegister(RDI), 8, false);

            if(src->isFrameIndex())
                x64Info->stackSlotAddress(block, block->getInstructions().size(), frame.getStackSlot(cast<MIR::FrameIndex>(src)->getIndex()), ri->getRegister(RSI));
            else
                instrInfo->move(block, block->getInstructions().size(), src, ri->getRegister(RSI), 8, false);

            if(len->isFrameIndex())
                x64Info->stackSlotAddress(block, block->getInstructions().size(), frame.getStackSlot(cast<MIR::FrameIndex>(len)->getIndex()), ri->getRegister(RCX));
            else
                instrInfo->move(block, block->getInstructions().size(), len, ri->getRegister(RCX), 8, false);

            block->addInstruction(instr(OPCODE(Rep_Movsb)));
            break;
        }
        default:
            break;
    }

    return ret;
}

bool matchGlobalValue(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitGlobalValue(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    GlobalValue* var = cast<GlobalValue>(i->getOperands().at(0));
    Unit* unit = block->getParentFunction()->getIRFunction()->getUnit();
    MIR::GlobalAddress* addr = var->getGlobal()->getMachineGlobalAddress(*unit);
    MIR::Operand* rr = isel->emitOrGet(i->getResult(), block);
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)instrInfo;
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    block->addInstruction(xInstrInfo->memoryToOperand(OPCODE(Lea64rm), rr, ri->getRegister(RIP), 0, nullptr, 1, addr));
    return rr;
}

bool matchZextTo64(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 8;
}

MIR::Operand* emitZextTo64(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    uint32_t opcode = selectOpcode(fromSize, false, {OPCODE(Movzx64r8r), OPCODE(Movzx64r16r), OPCODE(Mov32rr), 0}, {});
    if(fromSize == 4) { // force register to be 32bit to zero extend
        MIR::Register* ret32 = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(ret, Force32BitRegister)); // don't convert the original register
        block->addInstruction(instr(opcode, ret32, src));
    }
    else
        block->addInstruction(instr(opcode, ret, src));

    return ret;
}

bool matchZextTo32(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 4;
}

MIR::Operand* emitZextTo32(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    uint32_t opcode = selectOpcode(fromSize, false, {OPCODE(Movzx32r8r), OPCODE(Movzx32r16r), 0, 0}, {});
    block->addInstruction(instr(opcode, ret, src));

    return ret;
}

bool matchZextTo16(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 2;
}

MIR::Operand* emitZextTo16(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;
    block->addInstruction(instr(OPCODE(Movzx16r8r), ret, src));
    return ret;
}

bool matchSextTo64(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 8;
}

MIR::Operand* emitSextTo64(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    uint32_t opcode = selectOpcode(fromSize, false, {OPCODE(Movsx64r8r), OPCODE(Movsx64r16r), OPCODE(Movsx64r32r), 0}, {});
    block->addInstruction(instr(opcode, ret, src));

    return ret;
}

bool matchSextTo32(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 4;
}

MIR::Operand* emitSextTo32(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    uint32_t opcode = selectOpcode(fromSize, false, {OPCODE(Movsx32r8r), OPCODE(Movsx32r16r), 0, 0}, {});
    block->addInstruction(instr(opcode, ret, src));

    return ret;
}

bool matchSextTo16(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 2;
}

MIR::Operand* emitSextTo16(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    block->addInstruction(instr(OPCODE(Movsx16r8r), ret, src));

    return ret;
}

bool matchTrunc(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitTrunc(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t toSize = layout->getSize(cast<Cast>(i)->getType());
    MIR::Register* srcClone = nullptr;
    if(toSize == 1) srcClone = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(src, Force8BitRegister));
    else if(toSize == 2) srcClone = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(src, Force16BitRegister));
    else if(toSize == 4) srcClone = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(src, Force32BitRegister));
    instrInfo->move(block, block->last(), srcClone, ret, toSize, false);

    return ret;
}

bool matchFptrunc(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitFptrunc(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    block->addInstruction(instr(OPCODE(Cvtsd2ssrr), ret, src));
    return ret;
}

bool matchFpext(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitFpext(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    block->addInstruction(instr(OPCODE(Cvtss2sdrr), ret, src));
    return ret;
}

bool matchFptosi(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitFptosi(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    FloatType* fromType = (FloatType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());
    IntegerType* toType = (IntegerType*)(i->getResult()->getType());

    block->addInstruction(instr(toType->getBits() == 32 ?
        (fromType->getBits() == 32 ? OPCODE(Cvtss2si32rr) : OPCODE(Cvtsd2si32rr)) :
        (fromType->getBits() == 32 ? OPCODE(Cvtss2si64rr) : OPCODE(Cvtsd2si64rr)),
        ret, src
    ));
    
    return ret;
}

bool matchFptoui(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitFptoui(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    FloatType* fromType = (FloatType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());
    IntegerType* toType = (IntegerType*)(i->getResult()->getType());

    if(toType->getBits() == 32) {
        block->addInstruction(instr(fromType->getBits() == 32 ? OPCODE(Cvtss2si32rr) : OPCODE(Cvtsd2si32rr), ret, src));
    }
    else {
        throw std::runtime_error("Not legal");
    }
    
    return ret;
}

bool matchSitofp(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitSitofp(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    FloatType* toType = (FloatType*)(i->getResult()->getType());
    IntegerType* fromType = (IntegerType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());
    block->addInstruction(instr(toType->getBits() == 32 ?
        (fromType->getBits() == 32 ? OPCODE(Cvtsi2ss32rr) : OPCODE(Cvtsi2ss64rr)) :
        (fromType->getBits() == 32 ? OPCODE(Cvtsi2sd32rr) : OPCODE(Cvtsi2sd64rr)),
        ret, src
    ));
    
    return ret;
}

bool matchUitofp(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitUitofp(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    IntegerType* fromType = (IntegerType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());
    FloatType* toType = (FloatType*)(i->getResult()->getType());

    if(fromType->getBits() == 32) {
        block->addInstruction(instr(toType->getBits() == 32 ? OPCODE(Cvtsi2ss32rr) : OPCODE(Cvtsi2sd32rr), ret, src));
    }
    else {
        throw std::runtime_error("Not legal");
    }
    
    return ret;
}

bool matchShiftLeftImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitShiftLeftImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::ImmediateInt* right = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);
    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Shl8ri), OPCODE(Shl16ri), OPCODE(Shl32ri), OPCODE(Shl64ri)}, {});
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchShiftLeftRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && isRegister(extractOperand(i->getOperands().at(1)));
}
MIR::Operand* emitShiftLeftRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    right = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(right, Force8BitRegister));
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    instrInfo->move(block, block->last(), right, ri->getRegister(CL), 1, false);

    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Shl8rCL), OPCODE(Shl16rCL), OPCODE(Shl32rCL), OPCODE(Shl64rCL)}, {});
    block->addInstruction(instr(op, ret, ri->getRegister(CL)));

    return ret;
}

bool matchLShiftRightImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitLShiftRightImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::ImmediateInt* right = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);
    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Shr8ri), OPCODE(Shr16ri), OPCODE(Shr32ri), OPCODE(Shr64ri)}, {});
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchLShiftRightRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && isRegister(extractOperand(i->getOperands().at(1)));
}
MIR::Operand* emitLShiftRightRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    right = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(right, Force8BitRegister));
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    instrInfo->move(block, block->last(), right, ri->getRegister(CL), 1, false);

    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Shr8rCL), OPCODE(Shr16rCL), OPCODE(Shr32rCL), OPCODE(Shr64rCL)}, {});
    block->addInstruction(instr(op, ret, ri->getRegister(CL)));

    return ret;
}

bool matchAShiftRightImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitAShiftRightImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::ImmediateInt* right = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);
    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Sar8ri), OPCODE(Sar16ri), OPCODE(Sar32ri), OPCODE(Sar64ri)}, {});
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchAShiftRightRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && isRegister(extractOperand(i->getOperands().at(1)));
}
MIR::Operand* emitAShiftRightRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    right = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(right, Force8BitRegister));
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    instrInfo->move(block, block->last(), right, ri->getRegister(CL), 1, false);

    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Sar8rCL), OPCODE(Sar16rCL), OPCODE(Sar32rCL), OPCODE(Sar64rCL)}, {});
    block->addInstruction(instr(op, ret, ri->getRegister(CL)));

    return ret;
}

bool matchAndImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitAndImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(And8ri), OPCODE(And16ri), OPCODE(And32ri), OPCODE(And64rr)}, {});

    // TODO use r/m64, imm8 if imm is small enough
    if(op == (uint32_t)Opcode::And64rr) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        instrInfo->move(block, block->last(), right, reserved, 8, false);
        right = reserved;
    }
    
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchAndRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(1)));
}
MIR::Operand* emitAndRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(And8rr), OPCODE(And16rr), OPCODE(And32rr), OPCODE(And64rr)}, {});
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchOrImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitOrImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Or8ri), OPCODE(Or16ri), OPCODE(Or32ri), OPCODE(Or64rr)}, {});

    // TODO use r/m64, imm8 if imm is small enough
    if(op == (uint32_t)Opcode::Or64rr) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        instrInfo->move(block, block->last(), right, reserved, 8, false);
        right = reserved;
    }
    
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchOrRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(1)));
}
MIR::Operand* emitOrRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Or8rr), OPCODE(Or16rr), OPCODE(Or32rr), OPCODE(Or64rr)}, {});
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchXorImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}

MIR::Operand* emitXorImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Xor8ri), OPCODE(Xor16ri), OPCODE(Xor32ri), OPCODE(Xor64rr)}, {});

    // TODO use r/m64, imm8 if imm is small enough
    if(op == (uint32_t)Opcode::Or64rr) {
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back());
        instrInfo->move(block, block->last(), right, reserved, 8, false);
        right = reserved;
    }
    
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchXorRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(1)));
}

MIR::Operand* emitXorRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), left, ret, fromSize, false);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Xor8rr), OPCODE(Xor16rr), OPCODE(Xor32rr), OPCODE(Xor64rr)}, {});
    block->addInstruction(instr(op, ret, right));
    return ret;
}

bool matchIDiv(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitIDiv(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(
        instrInfo->getRegisterInfo()->getClassFromType(i->getResult()->getType())
    ).back());
    MIR::Register* rax = ri->getRegister(selectRegister(size, {AL, AX, EAX, RAX}));

    instrInfo->move(block, block->last(), left, rax, size, false);
    instrInfo->move(block, block->last(), right, reserved, size, false);

    uint32_t opcode = selectOpcode(size, false, {OPCODE(Cbw), OPCODE(Cwd), OPCODE(Cdq), OPCODE(Cqo)}, {});
    block->addInstruction(instr(opcode));

    opcode = selectOpcode(size, false, {OPCODE(IDiv8), OPCODE(IDiv16), OPCODE(IDiv32), OPCODE(IDiv64)}, {});
    block->addInstruction(instr(opcode, reserved));

    return rax;
}

bool matchUDiv(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitUDiv(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(
        instrInfo->getRegisterInfo()->getClassFromType(i->getResult()->getType())
    ).back());
    MIR::Register* rdx = ri->getRegister(selectRegister(size, {DL, DX, EDX, RDX}));
    MIR::Register* rax = ri->getRegister(selectRegister(size, {AL, AX, EAX, RAX}));

    instrInfo->move(block, block->last(), left, rax, size, false);
    instrInfo->move(block, block->last(), right, reserved, size, false);

    block->addInstruction(instr(OPCODE(Xor64rr), rdx, rdx));

    uint32_t opcode = selectOpcode(size, false, {OPCODE(Div8), OPCODE(Div16), OPCODE(Div32), OPCODE(Div64)}, {});
    block->addInstruction(instr(opcode, reserved));

    return rax;
}

bool matchFDiv(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitFDiv(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));

    instrInfo->move(block, block->last(), left, ret, size, true);
    block->addInstruction(instr(size == 4 ? OPCODE(Divssrr) : OPCODE(Divsdrr), ret, right));
    return ret;
}

bool matchIRem(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitIRem(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(
        instrInfo->getRegisterInfo()->getClassFromType(i->getResult()->getType())
    ).back());
    MIR::Register* rdx = ri->getRegister(selectRegister(size, {DL, DX, EDX, RDX}));
    MIR::Register* rax = ri->getRegister(selectRegister(size, {AL, AX, EAX, RAX}));

    instrInfo->move(block, block->last(), left, rax, size, false);
    instrInfo->move(block, block->last(), right, reserved, size, false);

    uint32_t opcode = selectOpcode(size, false, {OPCODE(Cbw), OPCODE(Cwd), OPCODE(Cdq), OPCODE(Cqo)}, {});
    block->addInstruction(instr(opcode));

    opcode = selectOpcode(size, false, {OPCODE(IDiv8), OPCODE(IDiv16), OPCODE(IDiv32), OPCODE(IDiv64)}, {});
    block->addInstruction(instr(opcode, reserved));

    return rdx;
}

bool matchURem(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitURem(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(
        instrInfo->getRegisterInfo()->getClassFromType(i->getResult()->getType())
    ).back());
    MIR::Register* rdx = ri->getRegister(selectRegister(size, {DL, DX, EDX, RDX}));
    MIR::Register* rax = ri->getRegister(selectRegister(size, {AL, AX, EAX, RAX}));

    instrInfo->move(block, block->last(), left, rax, size, false);
    instrInfo->move(block, block->last(), right, reserved, size, false);

    block->addInstruction(instr(OPCODE(Xor64rr), rdx, rdx));

    uint32_t opcode = selectOpcode(size, false, {OPCODE(Div8), OPCODE(Div16), OPCODE(Div32), OPCODE(Div64)}, {});
    block->addInstruction(instr(opcode, reserved));

    return rdx;
}

bool matchIMulRegisterImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        (isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt)
            ||
        (extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt && isRegister(extractOperand(i->getOperands().at(1))));
}
MIR::Operand* emitIMulRegisterImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    bool swap = extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt;
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block));
    MIR::ImmediateInt* rhs = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));

    uint32_t opcode = selectOpcode(size, false, {0, OPCODE(IMul16rri), OPCODE(IMul32rri), OPCODE(IMul64rri)}, {});
    block->addInstruction(instr(opcode, ret, lhs, rhs));
    
    return ret;
}

bool matchIMulRegisterRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 && isRegister(extractOperand(i->getOperands().at(0))) && isRegister(extractOperand(i->getOperands().at(1)));
}
MIR::Operand* emitIMulRegisterRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* rhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));

    instrInfo->move(block, block->last(), lhs, ret, size, false);
    uint32_t opcode = selectOpcode(size, false, {0, OPCODE(IMul16rr), OPCODE(IMul32rr), OPCODE(IMul64rr)}, {});
    block->addInstruction(instr(opcode, ret, rhs));

    return ret;
}

bool matchUMul(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitUMul(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* reserved = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(
        instrInfo->getRegisterInfo()->getClassFromType(i->getResult()->getType())
    ).back());
    MIR::Register* rax = ri->getRegister(selectRegister(size, {AL, AX, EAX, RAX}));

    instrInfo->move(block, block->last(), left, rax, size, false);
    instrInfo->move(block, block->last(), right, reserved, size, false);

    uint32_t opcode = selectOpcode(size, false, {OPCODE(Mul8), OPCODE(Mul16), OPCODE(Mul32), OPCODE(Mul64)}, {});
    block->addInstruction(instr(opcode, reserved));

    return rax;
}

bool matchFMul(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitFMul(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    size_t size = layout->getSize(i->getResult()->getType());

    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));

    instrInfo->move(block, block->last(), left, ret, size, true);
    block->addInstruction(instr(size == 4 ? OPCODE(Mulssrr) : OPCODE(Mulsdrr), ret, right));
    return ret;
}

bool matchGenericCast(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitGenericCast(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isel->emitOrGet(i->getOperands().at(0), block);
}

}
