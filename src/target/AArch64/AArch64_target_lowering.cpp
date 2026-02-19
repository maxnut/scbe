#include "target/AArch64/AArch64_target_lowering.hpp"
#include "IR/block.hpp"
#include "IR/global_value.hpp"
#include "MIR/function.hpp"
#include "IR/function.hpp"
#include "MIR/operand.hpp"
#include "MIR/register_info.hpp"
#include "MIR/stack_slot.hpp"
#include "target/AArch64/AArch64_instruction_info.hpp"
#include "target/AArch64/AArch64_register_info.hpp"
#include "target/call_info.hpp"
#include "target/instruction_info.hpp"
#include "target/instruction_utils.hpp"
#include "unit.hpp"

#include <deque>

namespace scbe::Target::AArch64 {

struct ArgInfo {
    MIR::Operand* op;
    Type* type;
    Ref<ArgAssign> assign;
};

MIR::CallInstruction* AArch64TargetLowering::lowerCall(MIR::Block* block, MIR::CallLowering* callLower) {
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)m_instructionInfo;
    size_t inIdx = block->getInstructionIdx(callLower);
    size_t begin = inIdx;
    std::unique_ptr<MIR::CallLowering> instruction = std::unique_ptr<MIR::CallLowering>(
        cast<MIR::CallLowering>(block->removeInstruction(callLower).release())
    );

    CallInfo info(m_registerInfo, m_dataLayout);

    CallingConvention cc = callLower->getCallingConvention();
    if(cc == CallingConvention::Count) cc = getDefaultCallingConvention(m_targetSpec);

    CallConvFunction ccfunc = getCCFunction(cc);
    info.analyzeCallOperands(ccfunc, instruction.get());

    std::deque<ArgInfo> args;
    std::vector<uint32_t> registers;

    for(size_t i = 2; i < instruction->getOperands().size(); i++) {
        MIR::Operand* op = instruction->getOperands().at(i);
        Type* type = instruction->getTypes().at(i - 1);
        Ref<ArgAssign> assign = info.getArgAssigns().at(i - 2);
        if(!op->isRegister() && !op->isImmediateInt() && !op->isFrameIndex())
            throw std::runtime_error("TODO Unsupported argument type");

        args.emplace_back(op, type, assign);
        if(!op->isRegister()) continue;
        registers.push_back(cast<MIR::Register>(op)->getId());
    }

    while(!args.empty()) {
        ArgInfo info = args.front();
        args.pop_front();

        auto assign = info.assign;
        auto op = info.op;
        auto type = info.type;

        if(Ref<RegisterAssign> ra = std::dynamic_pointer_cast<RegisterAssign>(assign)) {
            bool found = false;
            for(uint32_t reg : registers) {
                if(!m_registerInfo->isSameRegister(reg, ra->getRegister())) continue;
                found = true;
                break;
            }
            if(found) {
                args.push_back(info);
                continue;
            }

            if(op->isFrameIndex()) 
                inIdx += ((AArch64InstructionInfo*)m_instructionInfo)->stackSlotAddress(block, inIdx,
                    block->getParentFunction()->getStackFrame().getStackSlot(cast<MIR::FrameIndex>(op)->getIndex()), m_registerInfo->getRegister(ra->getRegister()));
            else
                inIdx += m_instructionInfo->move(block, inIdx, op, m_registerInfo->getRegister(ra->getRegister()), m_dataLayout->getSize(type), type->isFltType());
        }
        else if(Ref<StackAssign> sa = std::dynamic_pointer_cast<StackAssign>(assign)) {
            MIR::StackSlot slot = block->getParentFunction()->getStackFrame().addStackSlot(m_dataLayout->getSize(type), m_dataLayout->getAlignment(type));
            if(op->isRegister()) {
                inIdx += m_instructionInfo->registerToStackSlot(block, inIdx, cast<MIR::Register>(op),
                    slot);
            }
            if(op->isFrameIndex()) {
                MIR::Register* reserved = m_registerInfo->getRegister(m_registerInfo->getReservedRegisters(RegisterClass::GPR64).back());
                inIdx += ((AArch64InstructionInfo*)m_instructionInfo)->stackSlotAddress(block, inIdx,
                    block->getParentFunction()->getStackFrame().getStackSlot(cast<MIR::FrameIndex>(op)->getIndex()), reserved);
                inIdx += m_instructionInfo->registerToStackSlot(block, inIdx, reserved, slot);
            }
            else {
                inIdx += m_instructionInfo->immediateToStackSlot(block, inIdx, cast<MIR::ImmediateInt>(op),
                    slot);
            }
        }

        if(op->isRegister())
            registers.erase(std::remove_if(registers.begin(), registers.end(), [&](uint32_t r) { return m_registerInfo->isSameRegister(r, cast<MIR::Register>(op)->getId()); }));
    }

    if(info.getRetAssigns().size() == 1 && info.getRetAssigns().at(0)->getKind() == ArgAssign::Kind::Stack)
        block->getParentFunction()->getStackFrame().addStackSlot(m_dataLayout->getSize(instruction->getTypes().at(0)), m_dataLayout->getAlignment(instruction->getTypes().at(0)));
    
    size_t callPos = inIdx;
    auto callTarget = instruction->getOperands().at(1);
    uint32_t opcode = callTarget->isGlobalAddress() || callTarget->isExternalSymbol() ? (uint32_t)Opcode::Call : (uint32_t)Opcode::Call64r;
    auto callU = std::make_unique<MIR::CallInstruction>(opcode, instruction->getOperands().at(1));
    auto call = callU.get();
    call->setStartOffset(block->getInstructions().at(begin).get());
    block->addInstructionAt(std::move(callU), inIdx++);

    if(instruction->getOperands().at(0) && info.getRetAssigns().size() > 0) {
        MIR::Operand* op = instruction->getOperands().at(0);
        if(!op->isRegister() && !op->isMultiValue()) throw std::runtime_error("Unsupported return type");

        Type* type = instruction->getTypes().at(0);
        for(size_t i = 0; i < info.getRetAssigns().size(); i++) {
            MIR::Operand* operand = op->isMultiValue() ? cast<MIR::MultiValue>(op)->getValues().at(i) : op;
            Ref<ArgAssign> ret = info.getRetAssigns().at(i);
            if(Ref<RegisterAssign> ra = std::dynamic_pointer_cast<RegisterAssign>(ret)) {
                uint32_t classid = m_registerInfo->getRegisterIdClass(ra->getRegister(), block->getParentFunction()->getRegisterInfo());
                inIdx += m_instructionInfo->move(block, inIdx, m_registerInfo->getRegister(ra->getRegister()), operand, ra->getSize(), classid == FPR32 || classid == FPR64);
                call->addReturnRegister(ra->getRegister());
            }
            else if(Ref<StackAssign> sa = std::dynamic_pointer_cast<StackAssign>(ret)) {
                MIR::StackFrame& frame = block->getParentFunction()->getStackFrame();
                MIR::StackSlot slot = frame.getStackSlot(frame.getNumStackSlots() - 1);
                if(operand->isRegister()) {
                    inIdx += m_instructionInfo->stackSlotToRegister(block, inIdx, cast<MIR::Register>(operand),
                        slot);
                }
            }
        }
    }
    call->setEndOffset(block->getInstructions().at(inIdx-1).get());
    return call;
}
void AArch64TargetLowering::lowerFunction(MIR::Function* function) {
    CallInfo info(m_registerInfo, m_dataLayout);
    CallingConvention cc = function->getIRFunction()->getCallingConvention();
    if(cc == CallingConvention::Count) cc = getDefaultCallingConvention(m_targetSpec);

    CallConvFunction ccfunc = getCCFunction(cc);
    info.analyzeFormalArgs(ccfunc, function);
    int64_t stackOffset = 0;

    for(size_t i = 0; i < function->getArguments().size(); i++) {
        Ref<ArgAssign> assign = info.getArgAssigns().at(i);
        if(Ref<RegisterAssign> ra = std::dynamic_pointer_cast<RegisterAssign>(assign)) {
            function->addLiveIn(ra->getRegister());
            if(function->getArguments().at(i))
                function->replace(function->getArguments().at(i), m_registerInfo->getRegister(ra->getRegister()), true);

            uint32_t rclass = m_registerInfo->getRegisterDesc(ra->getRegister()).getRegClass();
            if(rclass == FPR32 || rclass == FPR64) m_usedFp++;
            else m_usedGp++;
        }
        else if(Ref<StackAssign> sa = std::dynamic_pointer_cast<StackAssign>(assign)) {
            Type* type = function->getIRFunction()->getArguments().at(i)->getType();
            stackOffset -= m_dataLayout->getSize(type);
            MIR::StackSlot slot(m_dataLayout->getSize(type), stackOffset, m_dataLayout->getAlignment(type));
            m_spiller.spill(cast<MIR::Register>(function->getArguments().at(i)), function, slot);
        }
    }

    MIR::StackFrame& stack = function->getStackFrame();

    if(function->getIRFunction()->getFunctionType()->isVarArg()) {
        function->getStackFrame().addStackSlot(((8 - m_usedGp) * 8) + ((8 - m_usedFp) * 16), 16); // reg save area
    }

    size_t size = stack.getSize();
    size_t rem = size % 16;
    if(rem != 0)
        size += 16 - rem;

    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)m_instructionInfo;

    MIR::Block* entryBlock = function->getEntryBlock();
    size_t beg = entryBlock->getInstructions().size();
    aInstrInfo->registersMemoryOp(entryBlock, 0, Opcode::StoreP64rm, {m_registerInfo->getRegister(X29), m_registerInfo->getRegister(X30)}, m_registerInfo->getRegister(SP), -16, Indexing::PreIndexed);
    aInstrInfo->move(entryBlock, 1, m_registerInfo->getRegister(SP), m_registerInfo->getRegister(X29), 8, false);

    Ref<Context> ctx = function->getIRFunction()->getUnit()->getContext();

    if(function->getIRFunction()->getFunctionType()->isVarArg()) {
        MIR::StackSlot vaArea = function->getStackFrame().getStackSlot(function->getStackFrame().getNumStackSlots() - 1);
        int64_t off = vaArea.m_offset;
        size_t cur = 2;
        MIR::Register* sp = m_registerInfo->getRegister(SP);
        if(m_usedGp < 8) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X7), sp, off);
        if(m_usedGp < 7) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X6), sp, off);
        if(m_usedGp < 6) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X5), sp, off);
        if(m_usedGp < 5) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X4), sp, off);
        if(m_usedGp < 4) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X3), sp, off);
        if(m_usedGp < 3) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X2), sp, off);
        if(m_usedGp < 2) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X1), sp, off);
        if(m_usedGp < 1) off -= 8; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(X0), sp, off);
        if(m_usedFp < 8) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q7), sp, off);
        if(m_usedFp < 7) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q6), sp, off);
        if(m_usedFp < 6) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q5), sp, off);
        if(m_usedFp < 5) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q4), sp, off);
        if(m_usedFp < 4) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q3), sp, off);
        if(m_usedFp < 3) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q2), sp, off);
        if(m_usedFp < 2) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q1), sp, off);
        if(m_usedFp < 1) off -= 16; aInstrInfo->registerMemoryOp(entryBlock, cur++, Opcode::Store64rm, m_registerInfo->getRegister(Q0), sp, off);
    }

    if(size > 0) {
        MIR::Operand* sizeOp = aInstrInfo->getImmediate(entryBlock, ctx->getImmediateInt(size, immSizeFromValue(size)));
        if(sizeOp->isRegister()) {
            MIR::Register* sizeReg = cast<MIR::Register>(sizeOp);
            sizeReg = m_registerInfo->getRegister(aInstrInfo->getRegisterInfo()->getRegisterWithSize(sizeReg->getId(), 8).value());
            entryBlock->addInstructionAt(instr((uint32_t)Opcode::Sub64rr, m_registerInfo->getRegister(SP), m_registerInfo->getRegister(SP), sizeReg), 2);
        }
        else {
            entryBlock->addInstructionAt(instr((uint32_t)Opcode::Sub64ri, m_registerInfo->getRegister(SP), m_registerInfo->getRegister(SP), sizeOp), 2);
        }
    }
    function->setFunctionPrologueSize(entryBlock->getInstructions().size() - beg);

    for(MIR::Instruction* ret : m_returnInstructions) {
        MIR::Block* bb = ret->getParentBlock();
        size_t beg = bb->getInstructions().size();
        size_t idx = bb->getInstructionIdx(ret);

        if(size > 0) {
            MIR::Operand* sizeOp = aInstrInfo->getImmediate(bb, idx, ctx->getImmediateInt(size, immSizeFromValue(size)));
            if(sizeOp->isRegister()) {
                MIR::Register* sizeReg = cast<MIR::Register>(sizeOp);
                sizeReg = m_registerInfo->getRegister(aInstrInfo->getRegisterInfo()->getRegisterWithSize(sizeReg->getId(), 8).value());
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64rr, m_registerInfo->getRegister(SP), m_registerInfo->getRegister(SP), sizeReg), idx++);
            }
            else {
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64ri, m_registerInfo->getRegister(SP), m_registerInfo->getRegister(SP), sizeOp), idx++);
            }
        }
        idx += aInstrInfo->registersMemoryOp(bb, idx, Opcode::LoadP64rm, {m_registerInfo->getRegister(X29), m_registerInfo->getRegister(X30)}, m_registerInfo->getRegister(SP), 16, Indexing::PostIndexed);
        bb->setEpilogueSize(bb->getInstructions().size() - beg);
    }

    m_returnInstructions.clear();
}

void AArch64TargetLowering::lowerSwitch(MIR::Block* block, MIR::SwitchLowering* lowering) {
    size_t inIdx = block->getInstructionIdx(lowering);
    std::unique_ptr<MIR::SwitchLowering> instruction = std::unique_ptr<MIR::SwitchLowering>(
        cast<MIR::SwitchLowering>(block->removeInstruction(lowering).release())
    );

    auto cases = instruction->getCases();
    auto minmax = std::minmax_element(cases.begin(), cases.end(), [](auto& a, auto& b) {
        MIR::ImmediateInt* left = a.first;
        MIR::ImmediateInt* right = b.first;
        return left->getValue() < right->getValue();
    });
    uint32_t min = minmax.first->first->getValue();
    uint32_t max = minmax.second->first->getValue();
    uint32_t span = max - min + 1;
    double density = static_cast<double>(instruction->getCases().size()) / static_cast<double>(span);

    constexpr double threshold = 0.5;
    if(density <= threshold) {
        // TODO
    }

    Unit* unit = block->getParentFunction()->getIRFunction()->getUnit();
    UMap<int64_t, IR::Block*> blocks;
    std::vector<IR::Constant*> table;
    for(auto& scase : instruction->getCases())
        blocks.insert({scase.first->getValue(), scase.second->getIRBlock()});

    for(size_t i = min; i <= max; i++) {
        if(!blocks.contains(i)) {
            table.push_back(instruction->getDefault()->getIRBlock());
            continue;
        }
        table.push_back(blocks.at(i));
    }
    Type* voidPtr = unit->getContext()->makePointerType(unit->getContext()->getVoidType());

    IR::ConstantArray* array = unit->getContext()->getConstantArray(unit->getContext()->makeArrayType(voidPtr, table.size()), table);
    IR::GlobalVariable* var = unit->getOrInsertGlobalVariable(voidPtr, array, IR::Linkage::Internal);

    MIR::GlobalAddress* addr = var->getMachineGlobalAddress(*unit);
    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)m_instructionInfo;
    MIR::RegisterInfo* mirRI;
    MIR::Register* rr = aInstrInfo->getRegisterInfo()->getRegister(
        block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(GPR64)
    );
    inIdx += aInstrInfo->getSymbolAddress(block, inIdx, addr, rr);
    MIR::Operand* index = instruction->getCondition();


    if(index->isImmediateInt()) {
        index = aInstrInfo->getImmediate(block, inIdx, cast<MIR::ImmediateInt>(index));
    }

    if(min != 0 || index->isImmediateInt()) {
        auto tmp = m_registerInfo->getRegister(
            block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(GPR64)
        );
        index = block->getParentFunction()->cloneOpWithFlags(index, Force64BitRegister);
        inIdx+= aInstrInfo->move(block, inIdx, index, tmp, 8, false);
        index = tmp;
    }
    else if(index->isRegister()) {
        index = block->getParentFunction()->cloneOpWithFlags(index, Force64BitRegister);
    }
    
    MIR::Operand* cmpImmMin = aInstrInfo->getImmediate(block, inIdx, unit->getContext()->getImmediateInt(min, MIR::ImmediateInt::imm32));
    if(cmpImmMin->isRegister()) cmpImmMin = block->getParentFunction()->cloneOpWithFlags(cmpImmMin, Force64BitRegister);
    uint32_t opcode = cmpImmMin->isImmediateInt() ? (uint32_t)Opcode::Subs64ri : (uint32_t)Opcode::Subs64rr;
    block->addInstructionAt(instr(opcode, aInstrInfo->getRegisterInfo()->getRegister(XZR), index, cmpImmMin), inIdx++);
    block->addInstructionAt(instr((uint32_t)Opcode::Blt, instruction->getDefault()), inIdx++);
    MIR::Operand* cmpImmMax = aInstrInfo->getImmediate(block, inIdx, unit->getContext()->getImmediateInt(max, MIR::ImmediateInt::imm32));
    if(cmpImmMax->isRegister()) cmpImmMax = block->getParentFunction()->cloneOpWithFlags(cmpImmMax, Force64BitRegister);
    opcode = cmpImmMax->isImmediateInt() ? (uint32_t)Opcode::Subs64ri : (uint32_t)Opcode::Subs64rr;
    block->addInstructionAt(instr(opcode, aInstrInfo->getRegisterInfo()->getRegister(XZR), index, cmpImmMax), inIdx++);
    block->addInstructionAt(instr((uint32_t)Opcode::Bgt, instruction->getDefault()), inIdx++);

    if(min != 0) {
        uint32_t opcode = cmpImmMin->isImmediateInt() ? (uint32_t)Opcode::Sub64ri : (uint32_t)Opcode::Sub64rr;
        MIR::Operand* cmpImmMin = aInstrInfo->getImmediate(block, inIdx, unit->getContext()->getImmediateInt(min, MIR::ImmediateInt::imm32));
        if(cmpImmMin->isRegister()) cmpImmMin = block->getParentFunction()->cloneOpWithFlags(cmpImmMin, Force64BitRegister);
        block->addInstructionAt(instr(opcode, index, index, cmpImmMin), inIdx++);
    }

    // lsl immediate by 3
    block->addInstructionAt(instr((uint32_t)Opcode::Ubfm64, index, index, unit->getContext()->getImmediateInt(61, MIR::ImmediateInt::imm8), unit->getContext()->getImmediateInt(60, MIR::ImmediateInt::imm8)), inIdx++);

    inIdx += aInstrInfo->registerMemoryOp(block, inIdx, Opcode::Load64rm, rr, rr, cast<MIR::Register>(index), Indexing::None, nullptr);
    block->addInstructionAt(instr((uint32_t)Opcode::Br, rr), inIdx++);
}

void AArch64TargetLowering::lowerReturn(MIR::Block* block, MIR::ReturnLowering* lowering) {
    CallingConvention cc = lowering->getParentBlock()->getParentFunction()->getIRFunction()->getCallingConvention();
    if(cc == CallingConvention::Count) cc = getDefaultCallingConvention(m_targetSpec);
    
    size_t inIdx = block->getInstructionIdx(lowering);
    std::unique_ptr<MIR::ReturnLowering> instruction = std::unique_ptr<MIR::ReturnLowering>(
        cast<MIR::ReturnLowering>(block->removeInstruction(lowering).release())
    );

    CallInfo info(m_registerInfo, m_dataLayout);

    CallConvFunction ccfunc = getCCFunction(cc);
    info.analyzeFormalArgs(ccfunc, lowering->getParentBlock()->getParentFunction());

    for(size_t i = 0; i < info.getRetAssigns().size(); i++) {
        auto& ret = info.getRetAssigns().at(i);
        if(auto regRet = dyn_cast<RegisterAssign>(ret)) {
            uint32_t classid = m_registerInfo->getRegisterIdClass(regRet->getRegister(), block->getParentFunction()->getRegisterInfo());
            if(lowering->getValue()->isRegister() || lowering->getValue()->isImmediateInt()) {
                inIdx += m_instructionInfo->move(block, inIdx, lowering->getValue(), m_registerInfo->getRegister(regRet->getRegister()), regRet->getSize(), classid == FPR32 || classid == FPR64);
            }
            else if(lowering->getValue()->isMultiValue()) {
                MIR::MultiValue* multi = cast<MIR::MultiValue>(lowering->getValue());
                inIdx += m_instructionInfo->move(block, inIdx, multi->getValues().at(i), m_registerInfo->getRegister(regRet->getRegister()), regRet->getSize(), classid == FPR32 || classid == FPR64);
            }
            else {
                throw std::runtime_error("Not implemented");
            }
        }
        else if(auto stackRet = dyn_cast<StackAssign>(ret)) {
            throw std::runtime_error("Not implemented");
        }
    }

    auto ret = instr((uint32_t)Opcode::Ret);
    m_returnInstructions.push_back(ret.get());
    block->addInstructionAt(std::move(ret), inIdx++);
}

void AArch64TargetLowering::lowerVaStart(MIR::Block* block, MIR::VaStartLowering* lowering) {
    size_t inIdx = block->getInstructionIdx(lowering);
    std::unique_ptr<MIR::VaStartLowering> instruction = std::unique_ptr<MIR::VaStartLowering>(
        cast<MIR::VaStartLowering>(block->removeInstruction(lowering).release())
    );

    AArch64InstructionInfo* aInstrInfo = (AArch64InstructionInfo*)m_instructionInfo;

    MIR::Register* base = lowering->getList()->isRegister() ? cast<MIR::Register>(lowering->getList()) : nullptr;
    if(!base && lowering->getList()->isFrameIndex()) {
        MIR::StackSlot slot = block->getParentFunction()->getStackFrame().getStackSlot(cast<MIR::FrameIndex>(lowering->getList())->getIndex());
        base = m_registerInfo->getRegister(m_registerInfo->getReservedRegisters(GPR64).back());
        inIdx += aInstrInfo->stackSlotAddress(block, inIdx, slot, base);
    }

    MIR::StackSlot regArea = block->getParentFunction()->getStackFrame().getStackSlot(block->getParentFunction()->getStackFrame().getNumStackSlots() - 1);
    MIR::Register* vreg = m_registerInfo->getRegister(
        block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(GPR64)
    );

    // we save sp into x29 in function prologue but only after pushing x29 and x30 so 16 bytes offsetted
    block->addInstruction(instr((uint32_t)Opcode::Add64ri, vreg, m_registerInfo->getRegister(X29), m_ctx->getImmediateInt(16, MIR::ImmediateInt::imm8)));
    inIdx += aInstrInfo->registerMemoryOp(block, inIdx, Opcode::Store64rm, vreg, base, 0l);

    inIdx += aInstrInfo->stackSlotAddress(block, inIdx, regArea, vreg);
    inIdx += aInstrInfo->registerMemoryOp(block, inIdx, Opcode::Store64rm, vreg, base, 8l);

    MIR::StackSlot fpArea(m_usedFp * 16, regArea.m_offset - (m_usedGp * 8), 16);
    inIdx += aInstrInfo->stackSlotAddress(block, inIdx, fpArea, vreg);
    inIdx += aInstrInfo->registerMemoryOp(block, inIdx, Opcode::Store64rm, vreg, base, 16l);

    MIR::Register* vreg2 = m_registerInfo->getRegister(
        block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(GPR32)
    );

    inIdx += aInstrInfo->move(block, inIdx, m_ctx->getImmediateInt(-8 * m_usedGp, MIR::ImmediateInt::imm8), vreg2, 4, false);
    inIdx += aInstrInfo->registerMemoryOp(block, inIdx, Opcode::Store64rm, vreg2, base, 24l);

    inIdx += aInstrInfo->move(block, inIdx, aInstrInfo->getImmediate(block, inIdx, m_ctx->getImmediateInt(-16 * m_usedFp, immSizeFromValue(-16 * m_usedFp))), vreg2, 4, false);
    inIdx += aInstrInfo->registerMemoryOp(block, inIdx, Opcode::Store64rm, vreg2, base, 28l);
}

void AArch64TargetLowering::lowerVaEnd(MIR::Block* block, MIR::VaEndLowering* lowering) {
}

}