#include "target/AArch64/AArch64_patterns.hpp"
#include "IR/function.hpp"
#include "IR/intrinsic.hpp"
#include "IR/value.hpp"
#include "MIR/instruction.hpp"
#include "MIR/operand.hpp"
#include "cast.hpp"
#include "codegen/dag_isel_pass.hpp"
#include "target/AArch64/AArch64_instruction_info.hpp"
#include "target/AArch64/AArch64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "ISel/DAG/instruction.hpp"
#include "MIR/function.hpp"
#include "target/register_info.hpp"
#include "type.hpp"
#include "unit.hpp"

namespace scbe::Target::AArch64 {

#define OPCODE(op) (uint32_t)Opcode::op

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
    return ri->getRegister(block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(reg->getType(), rclass));
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
    Opcode op = Opcode::Count;
    auto from = isel->emitOrGet(i->getOperands().at(1), block);
    auto frameIndex = ((FrameIndex*)extractOperand(i->getOperands().at(0)));
    MIR::StackSlot slot = block->getParentFunction()->getStackFrame().getStackSlot(frameIndex->getSlot());
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    if(from->isImmediateInt())
        from = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(from));
    
    if(from->isRegister()) {
        RegisterInfo* rinfo = instrInfo->getRegisterInfo();
        auto rr = cast<MIR::Register>(from);
        instrInfo->registerToStackSlot(block, block->getInstructions().size(), rr, slot);
    }
    else {
        auto imm = cast<MIR::ImmediateInt>(from);
        instrInfo->immediateToStackSlot(block, block->getInstructions().size(), imm, slot);
    }
    return nullptr;
}

bool matchStoreInPtrRegister(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().front()));
}

MIR::Operand* emitStoreInPtrRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    Opcode op = Opcode::Count;
    auto from = isel->emitOrGet(i->getOperands().at(1), block);
    auto rr = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    if(from->isImmediateInt()) {
        from = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(from));
        if(from->isImmediateInt()) {
            auto imm = cast<MIR::ImmediateInt>(from);
            RegisterInfo* ri = instrInfo->getRegisterInfo();
            MIR::Register* rr = ri->getRegister(aInstrInfo->getRegisterInfo()->getReservedRegisters(imm->getSize() == MIR::ImmediateInt::imm64 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), from, rr, imm->getSize(), false);
            from = rr;
        }
    }

    RegisterInfo* rinfo = instrInfo->getRegisterInfo();
    MIR::Register* fromRr = cast<MIR::Register>(from);
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(rr->getId(), block->getParentFunction()->getRegisterInfo());
    op = (Opcode)selectOpcode(rinfo->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Store32rm), OPCODE(Store32rm), OPCODE(Store32rm), OPCODE(Store64rm)}, {OPCODE(Store32rm), OPCODE(Store64rm)});
    aInstrInfo->registerMemoryOp(block, block->last(), op, fromRr, rr, (int64_t)0);

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
    MIR::Operand* result = isel->emitOrGet(i->getResult(), block);
    if(result->isRegister()) {
        instrInfo->stackSlotToRegister(block, block->last(), cast<MIR::Register>(result), slot);
    }
    else if(result->isMultiValue()) {
        MIR::MultiValue* multi = cast<MIR::MultiValue>(result);
        for(size_t i = 0; i < multi->getValues().size(); i++) {
            MIR::Register* value = cast<MIR::Register>(multi->getValues().at(i));
            size_t size = instrInfo->getRegisterInfo()->getRegisterClass(
                instrInfo->getRegisterInfo()->getRegisterIdClass(value->getId(), block->getParentFunction()->getRegisterInfo())
            ).getSize();
            instrInfo->stackSlotToRegister(block, block->last(), value, slot);
            slot.m_offset -= size;
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
    Opcode op = (Opcode)selectOpcode(layout, i->getResult()->getType(), {OPCODE(Load32rm), OPCODE(Load32rm), OPCODE(Load32rm), OPCODE(Load64rm)}, {OPCODE(Load32rm), OPCODE(Load64rm)});
    MIR::Operand* result = isel->emitOrGet(i->getResult(), block);
    auto rr = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    if(result->isRegister()) {
        aInstrInfo->registerMemoryOp(block, block->last(), op, cast<MIR::Register>(result), rr, (int64_t)0);
    }
    else if(result->isMultiValue()) {
        MIR::MultiValue* multi = cast<MIR::MultiValue>(result);
        for(size_t i = 0; i < multi->getValues().size(); i++) {
            MIR::Register* value = cast<MIR::Register>(multi->getValues().at(i));
            size_t size = instrInfo->getRegisterInfo()->getRegisterClass(
                instrInfo->getRegisterInfo()->getRegisterIdClass(value->getId(), block->getParentFunction()->getRegisterInfo())
            ).getSize();
            aInstrInfo->registerMemoryOp(block, block->last(), op, value, rr, i * size);
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

MIR::Operand* emitAddImmediates(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto c1 = (ConstantInt*)extractOperand(i->getOperands().at(0));
    auto c2 = (ConstantInt*)extractOperand(i->getOperands().at(1));
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    auto immediate = aInstrInfo->getImmediate(block, ctx->getImmediateInt(c1->getValue() + c2->getValue(), (MIR::ImmediateInt::Size)layout->getSize(c1->getType())));
    if(immediate->isRegister()) return immediate;
    
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
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* resultRegister = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    uint32_t op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Add32rr), OPCODE(Add32rr), OPCODE(Add32rr), OPCODE(Add64rr)}, {OPCODE(Fadd32rr), OPCODE(Fadd64rr)});
    block->addInstruction(instr(op, resultRegister, left, right));
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
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    bool swap = extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block));
    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block)));
    auto resultRegister = isel->emitOrGet(i->getResult(), block);
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    uint32_t op = 0;
    
    if(right->isImmediateInt())
        op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Add32ri), OPCODE(Add32ri), OPCODE(Add32ri), OPCODE(Add64ri)}, {OPCODE(Add32ri), OPCODE(Add64ri)});
    else
        op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Add32rr), OPCODE(Add32rr), OPCODE(Add32rr), OPCODE(Add64rr)}, {OPCODE(Add32rr), OPCODE(Add64rr)});

    block->addInstruction(instr(op, resultRegister, left, right));
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
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    auto immediate = aInstrInfo->getImmediate(block, ctx->getImmediateInt(c1->getValue() - c2->getValue(), (MIR::ImmediateInt::Size)layout->getSize(c1->getType())));
    if(immediate->isRegister()) return immediate;
    
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
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* resultRegister = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    uint32_t op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Sub32rr), OPCODE(Sub32rr), OPCODE(Sub32rr), OPCODE(Sub64rr)}, {OPCODE(Fsub32rr), OPCODE(Fsub64rr)});
    block->addInstruction(instr(op, resultRegister, left, right));
    return resultRegister;
}

bool matchSubRegisterImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        ((isRegister(extractOperand(i->getOperands().at(0))) &&
        extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt))
            ||
        (extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
        isRegister(extractOperand(i->getOperands().at(1))));

}

MIR::Operand* emitSubRegisterImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    bool swap = extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block));
    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block)));
    auto resultRegister = isel->emitOrGet(i->getResult(), block);
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    uint32_t op = 0;
    
    if(right->isImmediateInt())
        op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Sub32ri), OPCODE(Sub32ri), OPCODE(Sub32ri), OPCODE(Sub64ri)}, {OPCODE(Sub32ri), OPCODE(Sub64ri)});
    else
        op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Sub32rr), OPCODE(Sub32rr), OPCODE(Sub32rr), OPCODE(Sub64rr)}, {OPCODE(Sub32rr), OPCODE(Sub64rr)});

    block->addInstruction(instr(op, resultRegister, swap ? right : left, swap ? left : right));
    return resultRegister;
}

bool matchMulImmediates(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
        extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}

MIR::Operand* emitMulImmediates(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto c1 = (ConstantInt*)extractOperand(i->getOperands().at(0));
    auto c2 = (ConstantInt*)extractOperand(i->getOperands().at(1));
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    auto immediate = aInstrInfo->getImmediate(block, ctx->getImmediateInt(c1->getValue() * c2->getValue(), (MIR::ImmediateInt::Size)layout->getSize(c1->getType())));
    if(immediate->isRegister()) return immediate;
    
    auto returnReg = isel->emitOrGet(i->getResult(), block);
    instrInfo->move(block, block->last(), immediate, returnReg, layout->getSize(i->getResult()->getType()), false);
    return returnReg;
}

bool matchMulRegisters(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        isRegister(extractOperand(i->getOperands().at(0))) &&
        isRegister(extractOperand(i->getOperands().at(1)));
}

MIR::Operand* emitMulRegisters(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* resultRegister = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    uint32_t op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Mul32rr), OPCODE(Mul32rr), OPCODE(Mul32rr), OPCODE(Mul64rr)}, {OPCODE(Fmul32rr), OPCODE(Fmul64rr)});
    block->addInstruction(instr(op, resultRegister, left, right));
    return resultRegister;
}

bool matchMulRegisterImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() == 2 &&
        (isRegister(extractOperand(i->getOperands().at(0))) &&
         extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt) ||
        (extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt &&
         isRegister(extractOperand(i->getOperands().at(1))));
}

MIR::Operand* emitMulRegisterImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    bool swap = extractOperand(i->getOperands().at(0))->getKind() == Node::NodeKind::ConstantInt;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block));
    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block)));
    auto resultRegister = isel->emitOrGet(i->getResult(), block);
    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    uint32_t op = selectOpcode(instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize(), classid == FPR64 || classid == FPR32, {OPCODE(Mul32rr), OPCODE(Mul32rr), OPCODE(Mul32rr), OPCODE(Mul64rr)}, {OPCODE(Mul32rr), OPCODE(Mul64rr)});

    if(right->isImmediateInt()) {
        MIR::ImmediateInt* imm = cast<MIR::ImmediateInt>(right);
        RegisterInfo* ri = instrInfo->getRegisterInfo();
        MIR::Register* rr = ri->getRegister(aInstrInfo->getRegisterInfo()->getReservedRegisters(imm->getSize() == MIR::ImmediateInt::imm64 ? GPR64 : GPR32).back());
        aInstrInfo->move(block, block->last(), right, rr, imm->getSize(), false);
        right = rr;
    }

    block->addInstruction(instr(op, resultRegister, left, right));
    return resultRegister;
}

bool matchPhi(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getOperands().size() > 0;
}

MIR::Operand* emitPhi(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    auto result = isel->emitOrGet(extractOperand(i->getResult()), block);
    for(size_t idx = 0; idx < i->getOperands().size(); idx += 2) {
        auto tlabel = cast<MIR::Block>(isel->emitOrGet(i->getOperands().at(idx + 1), block));

        std::unique_ptr<MIR::Instruction> lastInstruction = nullptr;
        if(tlabel->getInstructions().size() > 0) {
            auto tmp = tlabel->getInstructions().at(tlabel->last() - 1).get();
            lastInstruction = tlabel->removeInstruction(tmp);
        }

        auto value = isel->emitOrGet(i->getOperands().at(idx), tlabel);
        auto type = ((ISel::DAG::Value*)extractOperand(i->getOperands().at(idx)))->getType();
        instrInfo->move(tlabel, tlabel->last(), value, result, layout->getSize(type), type->isFltType());
        if(lastInstruction)
            tlabel->addInstruction(std::move(lastInstruction));
    }
    return result;
}

bool matchJump(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return i->getKind() == Node::NodeKind::Jump && i->getOperands().size() == 1;
}

MIR::Operand* emitJump(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    block->addInstruction(instr(OPCODE(B), isel->emitOrGet(extractOperand(i->getOperands().at(0)), block)));
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
    uint32_t cmpOp = selectOpcode(fromSize, false, {OPCODE(Subs32ri), OPCODE(Subs32ri), OPCODE(Subs32ri), OPCODE(Subs64ri)}, {});
    block->addInstruction(instr(cmpOp, instrInfo->getRegisterInfo()->getRegister(fromSize > 4 ? XZR : WZR), cond, context->getImmediateInt(0, MIR::ImmediateInt::imm8)));
    block->addInstruction(instr(OPCODE(Bne), isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(B), isel->emitOrGet(i->getOperands().at(1), block)));
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
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;

    ConstantInt* constant = (ConstantInt*)extractOperand(comparison->getOperands().at(swap ? 0 : 1));
    size_t size = layout->getSize(constant->getType());
    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(comparison->getOperands().at(swap ? 0 : 1), block)));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(swap ? 1 : 0), block));

    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(right->isImmediateInt()) {
        size_t size = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();
        uint32_t op = selectOpcode(size, classid == FPR64 || classid == FPR32, {OPCODE(Subs32ri), OPCODE(Subs32ri), OPCODE(Subs32ri), OPCODE(Subs64ri)}, {});
        block->addInstruction(instr(op, ri->getRegister(size > 4 ? XZR : WZR), left, right));
    }
    else {
        size_t size = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();
        uint32_t op = selectOpcode(size, classid == FPR64 || classid == FPR32, {OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs64rr)}, {});
        block->addInstruction(instr(op, ri->getRegister(size > 4 ? XZR : WZR), left, right));
    }

    Opcode jmpOp;
    switch(comparison->getKind()) {
        case Node::NodeKind::ICmpEq:
            jmpOp = Opcode::Beq;
            break;
        case Node::NodeKind::ICmpNe:
            jmpOp = Opcode::Bne;
            break;
        case Node::NodeKind::ICmpGt:
            jmpOp = Opcode::Bgt;
            break;
        case Node::NodeKind::UCmpGt:
            jmpOp = Opcode::Bhi;
            break;
        case Node::NodeKind::ICmpGe:
            jmpOp = Opcode::Bge;
            break;
        case Node::NodeKind::UCmpGe:
            jmpOp = Opcode::Bcs;
            break;
        case Node::NodeKind::ICmpLt:
            jmpOp = Opcode::Blt;
            break;
        case Node::NodeKind::UCmpLt:
            jmpOp = Opcode::Bcc;
            break;
        case Node::NodeKind::ICmpLe:
            jmpOp = Opcode::Ble;
            break;
        case Node::NodeKind::UCmpLe:
            jmpOp = Opcode::Bls;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)jmpOp, isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(B), isel->emitOrGet(i->getOperands().at(1), block)));
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

    block->addInstruction(instr(OPCODE(B), isel->emitOrGet(i->getOperands().at(isTrue ? 0 : 1), block)));
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
    bool swap = comparison->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt;
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;


    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(swap ? 0 : 1), block));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(swap ? 1 : 0), block));

    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    size_t size = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();
    uint32_t op = selectOpcode(size, false, {OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs64rr)}, {});
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    block->addInstruction(instr(op, ri->getRegister(size > 4 ? XZR : WZR), left, right));

    Opcode jmpOp;
    switch(comparison->getKind()) {
        case Node::NodeKind::ICmpEq:
            jmpOp = Opcode::Beq;
            break;
        case Node::NodeKind::ICmpNe:
            jmpOp = Opcode::Bne;
            break;
        case Node::NodeKind::ICmpGt:
            jmpOp = Opcode::Bgt;
            break;
        case Node::NodeKind::UCmpGt:
            jmpOp = Opcode::Bhi;
            break;
        case Node::NodeKind::ICmpGe:
            jmpOp = Opcode::Bge;
            break;
        case Node::NodeKind::UCmpGe:
            jmpOp = Opcode::Bcs;
            break;
        case Node::NodeKind::ICmpLt:
            jmpOp = Opcode::Blt;
            break;
        case Node::NodeKind::UCmpLt:
            jmpOp = Opcode::Bcc;
            break;
        case Node::NodeKind::ICmpLe:
            jmpOp = Opcode::Ble;
            break;
        case Node::NodeKind::UCmpLe:
            jmpOp = Opcode::Bls;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)jmpOp, isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(B), isel->emitOrGet(i->getOperands().at(1), block)));
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
    bool swap = comparison->getOperands().at(0)->getKind() == Node::NodeKind::ConstantInt;
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;

    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(swap ? 0 : 1), block));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(comparison->getOperands().at(swap ? 1 : 0), block));

    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    size_t size = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();
    uint32_t op = selectOpcode(size, true, {}, {OPCODE(Fcmp32rr), OPCODE(Fcmp64rr)});
    block->addInstruction(instr(op, left, right));

    Opcode jmpOp;
    switch(comparison->getKind()) {
        case Node::NodeKind::FCmpEq:
            jmpOp = Opcode::Beq;
            break;
        case Node::NodeKind::FCmpNe:
            jmpOp = Opcode::Bne;
            break;
        case Node::NodeKind::FCmpGt:
            jmpOp = Opcode::Bhi;
            break;
        case Node::NodeKind::FCmpGe:
            jmpOp = Opcode::Bcs;
            break;
        case Node::NodeKind::FCmpLt:
            jmpOp = Opcode::Bcc;
            break;
        case Node::NodeKind::FCmpLe:
            jmpOp = Opcode::Bls;
            break;
        default:
            break;
    }

    block->addInstruction(instr((uint32_t)jmpOp, isel->emitOrGet(i->getOperands().at(0), block)));
    block->addInstruction(instr(OPCODE(B), isel->emitOrGet(i->getOperands().at(1), block)));
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
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;

    ConstantInt* constant = (ConstantInt*)extractOperand(i->getOperands().at(swap ? 0 : 1));
    size_t size = layout->getSize(constant->getType());

    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(swap ? 0 : 1), block)));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(swap ? 1 : 0), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));

    uint32_t classid = instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo());
    size_t rsize = instrInfo->getRegisterInfo()->getRegisterClass(classid).getSize();
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(right->isImmediateInt()) {
        uint32_t op = selectOpcode(rsize, classid == FPR64 || classid == FPR32, {OPCODE(Subs32ri), OPCODE(Subs32ri), OPCODE(Subs32ri), OPCODE(Subs64ri)}, {});
        block->addInstruction(instr(op, ri->getRegister(rsize > 4 ? XZR : WZR), left, right));
    }
    else {
        uint32_t op = selectOpcode(rsize, classid == FPR64 || classid == FPR32, {OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs64rr)}, {});
        block->addInstruction(instr(op, ri->getRegister(rsize > 4 ? XZR : WZR), left, right));
    }

    int64_t condition = 0; //csinc needs reverse condition
    switch(i->getKind()) {
        case Node::NodeKind::ICmpEq:
            condition = AArch64Conditions::Ne;
            break;
        case Node::NodeKind::ICmpNe:
            condition = AArch64Conditions::Eq;
            break;
        case Node::NodeKind::ICmpGt:
            condition = AArch64Conditions::Le;
            break;
        case Node::NodeKind::UCmpGt:
            condition = AArch64Conditions::Ls;
            break;
        case Node::NodeKind::ICmpGe:
            condition = AArch64Conditions::Lt;
            break;
        case Node::NodeKind::UCmpGe:
            condition = AArch64Conditions::Cc;
            break;
        case Node::NodeKind::ICmpLt:
            condition = AArch64Conditions::Ge;
            break;
        case Node::NodeKind::UCmpLt:
            condition = AArch64Conditions::Cs;
            break;
        case Node::NodeKind::ICmpLe:
            condition = AArch64Conditions::Gt;
            break;
        case Node::NodeKind::UCmpLe:
            condition = AArch64Conditions::Hi;
            break;
        default:
            break;
    }

    uint32_t csincOp = selectOpcode(rsize, false, {OPCODE(Csinc32), OPCODE(Csinc32), OPCODE(Csinc32), OPCODE(Csinc64)}, {});
    MIR::Register* zeroRegister = ri->getRegister(selectRegister(rsize, {WZR, WZR, WZR, XZR}));
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    MIR::ImmediateInt* conditionalImm = ctx->getImmediateInt(condition, MIR::ImmediateInt::imm8, AArch64OperandFlags::Conditional); 
    block->addInstruction(instr(csincOp, ret, zeroRegister, zeroRegister, conditionalImm));
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

    Opcode cmpOp = (Opcode)selectOpcode(fromSize, false, {OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs32rr), OPCODE(Subs64rr)}, {});
    block->addInstruction(instr((uint32_t)cmpOp, lhs, rhs));

    int64_t condition = 0; //csinc needs reverse condition
    switch(i->getKind()) {
        case Node::NodeKind::ICmpEq:
            condition = AArch64Conditions::Ne;
            break;
        case Node::NodeKind::ICmpNe:
            condition = AArch64Conditions::Eq;
            break;
        case Node::NodeKind::ICmpGt:
            condition = AArch64Conditions::Le;
            break;
        case Node::NodeKind::UCmpGt:
            condition = AArch64Conditions::Ls;
            break;
        case Node::NodeKind::ICmpGe:
            condition = AArch64Conditions::Lt;
            break;
        case Node::NodeKind::UCmpGe:
            condition = AArch64Conditions::Cc;
            break;
        case Node::NodeKind::ICmpLt:
            condition = AArch64Conditions::Ge;
            break;
        case Node::NodeKind::UCmpLt:
            condition = AArch64Conditions::Cs;
            break;
        case Node::NodeKind::ICmpLe:
            condition = AArch64Conditions::Gt;
            break;
        case Node::NodeKind::UCmpLe:
            condition = AArch64Conditions::Hi;
            break;
        default:
            break;
    }

    uint32_t csincOp = selectOpcode(fromSize, false, {OPCODE(Csinc32), OPCODE(Csinc32), OPCODE(Csinc32), OPCODE(Csinc64)}, {});
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* zeroRegister = ri->getRegister(selectRegister(fromSize, {WZR, WZR, WZR, XZR}));
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    MIR::ImmediateInt* conditionalImm = ctx->getImmediateInt(condition, MIR::ImmediateInt::imm8, AArch64OperandFlags::Conditional); 
    block->addInstruction(instr(csincOp, ret, zeroRegister, zeroRegister, conditionalImm));
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
    
    auto left = extractOperand(i->getOperands().at(0));
    auto right = extractOperand(i->getOperands().at(1));
    return isRegister(left) && cast<Value>(left)->getType()->isFltType() &&
        isRegister(right) && cast<Value>(right)->getType()->isFltType();
}
MIR::Operand* emitFCmpRegisterRegister(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);

    MIR::Register* lhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* rhs = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(lhs->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    
    FloatType* fromType = (FloatType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());

    Opcode cmpOp = (Opcode)selectOpcode(layout, fromType, {}, {OPCODE(Fcmp32rr), OPCODE(Fcmp64rr)});

    block->addInstruction(instr((uint32_t)cmpOp, lhs, rhs));

    int64_t condition = 0; //csinc needs reverse condition
    switch(i->getKind()) {
        case Node::NodeKind::FCmpEq:
            condition = AArch64Conditions::Ne;
            break;
        case Node::NodeKind::FCmpNe:
            condition = AArch64Conditions::Eq;
            break;
        case Node::NodeKind::FCmpGt:
            condition = AArch64Conditions::Ls;
            break;
        case Node::NodeKind::FCmpGe:
            condition = AArch64Conditions::Cc;
            break;
        case Node::NodeKind::FCmpLt:
            condition = AArch64Conditions::Cs;
            break;
        case Node::NodeKind::FCmpLe:
            condition = AArch64Conditions::Hi;
            break;
        default:
            break;
    }

    uint32_t csincOp = selectOpcode(fromSize, false, {OPCODE(Csinc32), OPCODE(Csinc32), OPCODE(Csinc32), OPCODE(Csinc64)}, {});
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    MIR::Register* zeroRegister = ri->getRegister(selectRegister(fromSize, {WZR, WZR, WZR, XZR}));
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    MIR::ImmediateInt* conditionalImm = ctx->getImmediateInt(condition, MIR::ImmediateInt::imm8, AArch64OperandFlags::Conditional); 
    block->addInstruction(instr(csincOp, ret, zeroRegister, zeroRegister, conditionalImm));
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

    MIR::Register* returnReg = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::GlobalAddress* symbol = IR::GlobalVariable::get(*unit, cnst->getType(), cnst, IR::Linkage::Internal)->getMachineGlobalAddress(*unit);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    
    aInstrInfo->getSymbolValue(block, block->last(), symbol, returnReg);
    return returnReg;
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
        base = ri->getRegister(X29);
    }

    for(size_t idx = 1; idx < i->getOperands().size(); idx++) {
        MIR::Operand* index = isel->emitOrGet(i->getOperands().at(idx), block);
        if(index->isImmediateInt()) {
            MIR::ImmediateInt* imm = (MIR::ImmediateInt*)index;
            curType = curType->isStructType()
                      ? curType->getContainedTypes().at(imm->getValue())
                      : curType->getContainedTypes().at(0);
            curOff += imm->getValue() * layout->getSize(curType);
            continue;
        }
        else {
            AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
            auto tmp = instrInfo->getRegisterInfo()->getRegister(
                block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(block->getParentFunction()->getIRFunction()->getUnit()->getContext()->getI64Type(), GPR64)
            );
            index = block->getParentFunction()->cloneOpWithFlags(index, Force64BitRegister);
            instrInfo->move(block, block->last(), index, tmp, 8, false);
            index = tmp;
            curOff = 0;
            size_t scale = layout->getSize(curType);
            MIR::Operand* scaleOp = aInstrInfo->getImmediate(block, context->getImmediateInt(scale, MIR::ImmediateInt::imm64));
            if(scaleOp->isImmediateInt()) {
                tmp = instrInfo->getRegisterInfo()->getRegister(
                    instrInfo->getRegisterInfo()->getReservedRegisters(GPR64).back()
                );
                instrInfo->move(block, block->last(), scaleOp, tmp, 8, false);
                scaleOp = tmp;
            }
            block->addInstruction(instr((uint32_t)Opcode::Mul64rr, index, index, scaleOp));
            block->addInstruction(instr((uint32_t)Opcode::Add64rr, base, base, index));
        }
    }

    auto size = immSizeFromValue(curOff);
    MIR::Operand* off = nullptr;
    uint32_t opcode = 0;
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    if(curOff != 0) {
        if(curOff < 0) {
            curOff *= -1;
            off = ((AArch64InstructionInfo*)instrInfo)->getImmediate(block, ctx->getImmediateInt(curOff, size));
            if(off->isRegister()) opcode = size <= 4 ? OPCODE(Sub32rr) : OPCODE(Sub64rr);
            else opcode = size <= 4 ? OPCODE(Sub32ri) : OPCODE(Sub64ri);
        }
        else {
            off = ((AArch64InstructionInfo*)instrInfo)->getImmediate(block, ctx->getImmediateInt(curOff, size));
            if(off->isRegister()) opcode = size <= 4 ? OPCODE(Add32rr) : OPCODE(Add64rr);
            else opcode = size <= 4 ? OPCODE(Add32ri) : OPCODE(Add64ri);
        }
        block->addInstruction(instr(opcode, ret, cast<MIR::Register>(base), off));
    }

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
    ins->addType(i->getResult()->getType());
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
            // TODO replace this call
            Unit* unit = block->getParentFunction()->getIRFunction()->getUnit();
            MIR::Operand* ret = i->getResult()->getType()->isVoidType() ? nullptr : isel->emitOrGet(i->getResult(), block);
            std::unique_ptr<MIR::CallLowering> ins = std::make_unique<MIR::CallLowering>();
            ins->addOperand(ret);
            ins->addType(i->getResult()->getType());
            ins->getOperands().push_back(unit->getOrInsertExternal("memcpy"));
            for(size_t idx = 0; idx < call->getOperands().size(); idx++) {
                ins->getOperands().push_back(isel->emitOrGet(i->getOperands().at(idx), block));
                auto op = cast<ISel::DAG::Value>(extractOperand(i->getOperands().at(idx)));
                ins->addType(op->getType());
            }
            block->addInstruction(std::move(ins));
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
    MIR::GlobalAddress* symbol = var->getGlobal()->getMachineGlobalAddress(*unit);
    MIR::Register* returnReg = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    aInstrInfo->getSymbolAddress(block, block->last(), symbol, returnReg);
    return returnReg;
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

    if(fromSize == 4) { // force register to be 32bit to zero extend
        MIR::Register* ret32 = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(ret, Force32BitRegister)); // don't convert the original register
        instrInfo->move(block, block->last(), src, ret32, 4, false);
    }
    else {
        MIR::Register* src64 = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(src, Force64BitRegister)); // don't convert the original register
        Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
        auto immed = ctx->getImmediateInt(fromSize == 1 ? 0xFF : 0xFFFF, (MIR::ImmediateInt::Size)fromSize);
        block->addInstruction(instr(OPCODE(And64ri), ret, src64, immed));
    }

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

    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    auto immed = ctx->getImmediateInt(fromSize == 1 ? 0xFF : 0xFFFF, (MIR::ImmediateInt::Size)fromSize);
    block->addInstruction(instr(OPCODE(And32ri), ret, src, immed));

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
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    block->addInstruction(instr(fromSize == 8 ? OPCODE(Ubfm64) : OPCODE(Ubfm32), ret, src, ctx->getImmediateInt(0, MIR::ImmediateInt::imm8), ctx->getImmediateInt(7, MIR::ImmediateInt::imm8)));
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

    MIR::Register* src64 = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(src, Force64BitRegister)); // don't convert the original register
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    block->addInstruction(instr(fromSize == 8 ? OPCODE(Sbfm64) : OPCODE(Sbfm32), ret, src64, ctx->getImmediateInt(0, MIR::ImmediateInt::imm8), ctx->getImmediateInt(fromSize * 8 - 1, MIR::ImmediateInt::imm8)));

    return ret;
}

bool matchSextTo32or16(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return layout->getSize(
        cast<Cast>(i)->getType()
    ) == 4;
}

MIR::Operand* emitSextTo32or16(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    if(src->isImmediateInt()) return src;

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(src->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();

    block->addInstruction(instr(fromSize == 8 ? OPCODE(Sbfm64) : OPCODE(Sbfm32), ret, src, ctx->getImmediateInt(0, MIR::ImmediateInt::imm8), ctx->getImmediateInt(fromSize * 8 - 1, MIR::ImmediateInt::imm8)));

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
    if(toSize == 4) {
        MIR::Register* src32 = cast<MIR::Register>(block->getParentFunction()->cloneOpWithFlags(src, Force32BitRegister)); // don't convert the original register
        instrInfo->move(block, block->last(), src32, ret, 4, false);
    }
    else {
        Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
        block->addInstruction(instr(toSize == 8 ? OPCODE(Ubfm64) : OPCODE(Ubfm32), ret, src, ctx->getImmediateInt(0, MIR::ImmediateInt::imm8), ctx->getImmediateInt(toSize * 8 - 1, MIR::ImmediateInt::imm8)));
    }

    return ret;
}

bool matchFptrunc(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitFptrunc(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    block->addInstruction(instr(OPCODE(Fcvt), ret, src));
    return ret;
}

bool matchFpext(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitFpext(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    block->addInstruction(instr(OPCODE(Fcvt), ret, src));
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
    uint32_t opcode = fromType->getBits() == 64 || toType->getBits() == 64 ? OPCODE(Fcvtzs64) : OPCODE(Fcvtzs32);

    block->addInstruction(instr(opcode, ret, src));
    
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
    uint32_t opcode = fromType->getBits() == 64 || toType->getBits() == 64 ? OPCODE(Fcvtzu64) : OPCODE(Fcvtzu32);

    block->addInstruction(instr(opcode, ret, src));
    
    return ret;
}

bool matchSitofp(MATCHER_ARGS) {
    return true;
}

MIR::Operand* emitSitofp(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* src = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));

    IntegerType* fromType = (IntegerType*)(cast<Value>(extractOperand(i->getOperands().at(0)))->getType());
    FloatType* toType = (FloatType*)(i->getResult()->getType());
    uint32_t opcode = fromType->getBits() == 64 || toType->getBits() == 64 ? OPCODE(Scvtf64) : OPCODE(Scvtf32);

    block->addInstruction(instr(opcode, ret, src));
    
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
    uint32_t opcode = fromType->getBits() == 64 || toType->getBits() == 64 ? OPCODE(Ucvtf64) : OPCODE(Ucvtf32);

    block->addInstruction(instr(opcode, ret, src));
    
    return ret;
}

bool matchShiftLeftImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitShiftLeftImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::ImmediateInt* right = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    if(fromSize < 4) fromSize = 4;

    int64_t rightValue = right->getValue() % (fromSize * 8);
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    right = ctx->getImmediateInt(fromSize - rightValue, right->getSize());
    block->addInstruction(instr(fromSize == 8 ? OPCODE(Ubfm64) : OPCODE(Ubfm32), ret, left, right, ctx->getImmediateInt(fromSize * 8 - 1, MIR::ImmediateInt::imm8)));
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
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Lslv32), OPCODE(Lslv32), OPCODE(Lslv32), OPCODE(Lslv64)}, {});
    block->addInstruction(instr(op, ret, left, right));

    return ret;
}

bool matchLShiftRightImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitLShiftRightImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::ImmediateInt* right = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    if(fromSize < 4) fromSize = 4;

    int64_t rightValue = right->getValue() % (fromSize * 8);
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    right = ctx->getImmediateInt(rightValue, right->getSize());
    block->addInstruction(instr(fromSize == 8 ? OPCODE(Ubfm64) : OPCODE(Ubfm32), ret, left, right, ctx->getImmediateInt(fromSize * 8 - 1, MIR::ImmediateInt::imm8)));
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
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Lsrv32), OPCODE(Lsrv32), OPCODE(Lsrv32), OPCODE(Lsrv64)}, {});
    block->addInstruction(instr(op, ret, left, right));

    return ret;
}

bool matchAShiftRightImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return isRegister(extractOperand(i->getOperands().at(0))) && extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitAShiftRightImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::ImmediateInt* right = cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    if(fromSize < 4) fromSize = 4;

    int64_t rightValue = right->getValue() % (fromSize * 8);
    Ref<Context> ctx = block->getParentFunction()->getIRFunction()->getUnit()->getContext();
    right = ctx->getImmediateInt(rightValue, right->getSize());
    block->addInstruction(instr(fromSize == 8 ? OPCODE(Sbfm64) : OPCODE(Sbfm32), ret, left, right, ctx->getImmediateInt(fromSize * 8 - 1, MIR::ImmediateInt::imm8)));
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
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Asrv32), OPCODE(Asrv32), OPCODE(Asrv32), OPCODE(Asrv64)}, {});
    block->addInstruction(instr(op, ret, left, right));

    return ret;
}

bool matchAndImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitAndImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block)));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    if(right->isRegister()) {
        uint32_t op = selectOpcode(fromSize, false, {OPCODE(And32rr), OPCODE(And32rr), OPCODE(And32rr), OPCODE(And64rr)}, {});
        block->addInstruction(instr(op, ret, left, right));
    }
    else {
        uint32_t op = selectOpcode(fromSize, false, {OPCODE(And32ri), OPCODE(And32ri), OPCODE(And32ri), OPCODE(And64ri)}, {});
        block->addInstruction(instr(op, ret, left, right));
    }

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
    uint32_t op = selectOpcode(fromSize, false, {OPCODE(And32rr), OPCODE(And32rr), OPCODE(And32rr), OPCODE(And64rr)}, {});
    block->addInstruction(instr(op, ret, left, right));
    return ret;
}

bool matchOrImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitOrImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block)));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    if(right->isRegister()) {
        uint32_t op = selectOpcode(fromSize, false, {OPCODE(Orr32rr), OPCODE(Orr32rr), OPCODE(Orr32rr), OPCODE(Orr64rr)}, {});
        block->addInstruction(instr(op, ret, left, right));
    }
    else {
        uint32_t op = selectOpcode(fromSize, false, {OPCODE(Orr32ri), OPCODE(Orr32ri), OPCODE(Orr32ri), OPCODE(Orr64ri)}, {});
        block->addInstruction(instr(op, ret, left, right));
    }

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
    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Orr32rr), OPCODE(Orr32rr), OPCODE(Orr32rr), OPCODE(Orr64rr)}, {});
    block->addInstruction(instr(op, ret, left, right));
    return ret;
}

bool matchXorImmediate(MATCHER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    return extractOperand(i->getOperands().at(1))->getKind() == Node::NodeKind::ConstantInt;
}
MIR::Operand* emitXorImmediate(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    size_t fromSize = instrInfo->getRegisterInfo()->getRegisterClass(
        instrInfo->getRegisterInfo()->getRegisterIdClass(left->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();

    MIR::Operand* right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(isel->emitOrGet(i->getOperands().at(1), block)));
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    if(right->isRegister()) {
        uint32_t op = selectOpcode(fromSize, false, {OPCODE(Eor32rr), OPCODE(Orr32rr), OPCODE(Eor32rr), OPCODE(Eor64rr)}, {});
        block->addInstruction(instr(op, ret, left, right));
    }
    else {
        uint32_t op = selectOpcode(fromSize, false, {OPCODE(Eor32ri), OPCODE(Eor32ri), OPCODE(Eor32ri), OPCODE(Eor64ri)}, {});
        block->addInstruction(instr(op, ret, left, right));
    }

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
    uint32_t op = selectOpcode(fromSize, false, {OPCODE(Eor32rr), OPCODE(Eor32rr), OPCODE(Eor32rr), OPCODE(Eor64rr)}, {});
    block->addInstruction(instr(op, ret, left, right));
    return ret;
}

bool matchIDiv(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitIDiv(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t size = layout->getSize((IntegerType*)(i->getResult()->getType()));

    if(left->isImmediateInt() && right->isImmediateInt()) throw std::runtime_error("Illegal");

    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(left->isImmediateInt()) {
        left = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(left));
        if(left->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), left, tmp, size, false);
            left = tmp;
        }
    }
    else if(right->isImmediateInt()) {
        right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(right));
        if(right->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), right, tmp, size, false);
            right = tmp;
        }
    }

    block->addInstruction(instr(size == 8 ? OPCODE(Sdiv64rr) : OPCODE(Sdiv32rr), ret, left, right));
    
    return ret;
}

bool matchUDiv(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitUDiv(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t size = layout->getSize((IntegerType*)(i->getResult()->getType()));

    if(left->isImmediateInt() && right->isImmediateInt()) throw std::runtime_error("Illegal");

    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(left->isImmediateInt()) {
        left = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(left));
        if(left->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), left, tmp, size, false);
            left = tmp;
        }
    }
    else if(right->isImmediateInt()) {
        right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(right));
        if(right->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 7 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), right, tmp, size, false);
            right = tmp;
        }
    }

    block->addInstruction(instr(size == 8 ? OPCODE(Udiv64rr) : OPCODE(Udiv32rr), ret, left, right));
    
    return ret;
}

bool matchFDiv(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitFDiv(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Register* ret = cast<MIR::Register>(isel->emitOrGet(i->getResult(), block));
    MIR::Register* left = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(0), block));
    MIR::Register* right = cast<MIR::Register>(isel->emitOrGet(i->getOperands().at(1), block));
    size_t size = layout->getSize((IntegerType*)(i->getResult()->getType()));

    block->addInstruction(instr(size == 8 ? OPCODE(Fdiv64rr) : OPCODE(Fdiv32rr), ret, left, right));
    return ret;
}

bool matchIRem(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitIRem(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t size = layout->getSize((IntegerType*)(i->getResult()->getType()));

    if(left->isImmediateInt() && right->isImmediateInt()) throw std::runtime_error("Illegal");

    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(left->isImmediateInt()) {
        left = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(left));
        if(left->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), left, tmp, size, false);
            left = tmp;
        }
    }
    else if(right->isImmediateInt()) {
        right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(right));
        if(right->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), right, tmp, size, false);
            right = tmp;
        }
    }

    block->addInstruction(instr(size == 8 ? OPCODE(Sdiv64rr) : OPCODE(Sdiv32rr), ret, left, right));
    block->addInstruction(instr(size == 8 ? OPCODE(Msub64) : OPCODE(Msub32), ret, ret, right, left));
    
    return ret;
}

bool matchURem(MATCHER_ARGS) {
    return true;
}
MIR::Operand* emitURem(EMITTER_ARGS) {
    ISel::DAG::Instruction* i = cast<ISel::DAG::Instruction>(node);
    MIR::Operand* left = isel->emitOrGet(i->getOperands().at(0), block);
    MIR::Operand* right = isel->emitOrGet(i->getOperands().at(1), block);
    MIR::Operand* ret = isel->emitOrGet(i->getResult(), block);

    size_t size = layout->getSize((IntegerType*)(i->getResult()->getType()));

    if(left->isImmediateInt() && right->isImmediateInt()) throw std::runtime_error("Illegal");

    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)instrInfo;
    RegisterInfo* ri = instrInfo->getRegisterInfo();
    if(left->isImmediateInt()) {
        left = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(left));
        if(left->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), left, tmp, size, false);
            left = tmp;
        }
    }
    else if(right->isImmediateInt()) {
        right = aInstrInfo->getImmediate(block, cast<MIR::ImmediateInt>(right));
        if(right->isImmediateInt()) {
            auto tmp = ri->getRegister(instrInfo->getRegisterInfo()->getReservedRegisters(size == 8 ? GPR64 : GPR32).back());
            aInstrInfo->move(block, block->last(), right, tmp, size, false);
            right = tmp;
        }
    }

    block->addInstruction(instr(size == 8 ? OPCODE(Udiv64rr) : OPCODE(Udiv32rr), ret, left, right));
    block->addInstruction(instr(size == 8 ? OPCODE(Msub64) : OPCODE(Msub32), ret, ret, right, left));
    
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