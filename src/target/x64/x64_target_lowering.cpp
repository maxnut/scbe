#include "target/x64/x64_target_lowering.hpp"
#include "IR/global_value.hpp"
#include "IR/value.hpp"
#include "IR/block.hpp"
#include "MIR/function.hpp"
#include "IR/function.hpp"
#include "MIR/instruction.hpp"
#include "MIR/operand.hpp"
#include "MIR/stack_frame.hpp"
#include "opt_level.hpp"
#include "target/call_info.hpp"
#include "target/instruction_info.hpp"
#include "target/x64/x64_calling_conventions.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "unit.hpp"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <vector>

namespace scbe::Target::x64 {

struct ArgInfo {
    MIR::Operand* op;
    Type* type;
    Ref<ArgAssign> assign;
};

void x64TargetLowering::lowerCall(MIR::Block* block, MIR::CallLowering* callLower) {
    size_t inIdx = block->getInstructionIdx(callLower);
    size_t begin = inIdx;
    std::unique_ptr<MIR::CallLowering> instruction = std::unique_ptr<MIR::CallLowering>(
        cast<MIR::CallLowering>(block->removeInstruction(callLower).release())
    );

    CallInfo info(m_registerInfo, m_dataLayout);

    CallConvFunction ccfunc = m_os == OS::Linux ? CCx64SysV : m_os == OS::Windows ? CCx64Win64 : nullptr;
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
                inIdx += ((x64InstructionInfo*)m_instructionInfo)->stackSlotAddress(block, inIdx,
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
                inIdx += ((x64InstructionInfo*)m_instructionInfo)->stackSlotAddress(block, inIdx,
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
    call->setStartOffset(inIdx - begin);
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
                size_t classsize = m_registerInfo->getRegisterClass(classid).getSize();
                inIdx += m_instructionInfo->move(block, inIdx, m_registerInfo->getRegister(ra->getRegister()), operand, ra->getSize(), classid == FPR);
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
    call->setEndOffset(inIdx - callPos);
}

void x64TargetLowering::lowerFunction(MIR::Function* function) {
    if(function->getIRFunction()->getHeuristics().getCallSites().size() > 0) function->getStackFrame().addStackSlot(16, 16);
    CallInfo info(m_registerInfo, m_dataLayout);
    CallConvFunction ccfunc = m_os == OS::Linux ? CCx64SysV : m_os == OS::Windows ? CCx64Win64 : nullptr;
    info.analyzeFormalArgs(ccfunc, function);
    int64_t stackOffset = 0;

    for(size_t i = 0; i < function->getArguments().size(); i++) {
        Ref<ArgAssign> assign = info.getArgAssigns().at(i);
        if(Ref<RegisterAssign> ra = std::dynamic_pointer_cast<RegisterAssign>(assign)) {
            function->addLiveIn(ra->getRegister());
            if(function->getArguments().at(i))
                function->replace(function->getArguments().at(i), m_registerInfo->getRegister(ra->getRegister()), true);
        }
        else if(Ref<StackAssign> sa = std::dynamic_pointer_cast<StackAssign>(assign)) {
            Type* type = function->getIRFunction()->getArguments().at(i)->getType();
            stackOffset -= m_dataLayout->getSize(type);
            MIR::StackSlot slot(m_dataLayout->getSize(type), stackOffset, m_dataLayout->getAlignment(type));
            m_spiller.spill(cast<MIR::Register>(function->getArguments().at(i)), function, slot);
        }
    }

    MIR::StackFrame& stack = function->getStackFrame();
    size_t size = stack.getSize();
    size_t rem = size % 16;
    if(rem != 0)
        size += 16 - rem;


    auto block = function->getEntryBlock();
    size_t beg = block->getInstructions().size();
    Ref<Context> ctx = function->getIRFunction()->getUnit()->getContext();

    block->addInstructionAtFront(instr((uint32_t)Opcode::Push64r, m_registerInfo->getRegister(x64::RegisterId::RBP)));
    block->addInstructionAt(instr((uint32_t)Opcode::Mov64rr, m_registerInfo->getRegister(x64::RegisterId::RBP), m_registerInfo->getRegister(x64::RegisterId::RSP)), 1);

    if(size > 0) {
        block->addInstructionAt(instr((uint32_t)Opcode::Mov64rr, m_registerInfo->getRegister(x64::RegisterId::RBP), m_registerInfo->getRegister(x64::RegisterId::RSP)), 1);
        if(size <= std::numeric_limits<int8_t>().max())
            block->addInstructionAt(instr((uint32_t)Opcode::Sub64r8i, m_registerInfo->getRegister(x64::RegisterId::RSP), ctx->getImmediateInt(size, MIR::ImmediateInt::imm8)), 2);
        else
            block->addInstructionAt(instr((uint32_t)Opcode::Sub64r32i, m_registerInfo->getRegister(x64::RegisterId::RSP), ctx->getImmediateInt(size, MIR::ImmediateInt::imm32)), 2);
    }
    function->setFunctionPrologueSize(block->getInstructions().size() - beg);

    assert(m_returnInstructions.size() > 0 && "Missing return instruction");
    for(MIR::Instruction* ret : m_returnInstructions) {
        MIR::Block* bb = ret->getParentBlock();
        size_t idx = bb->getInstructionIdx(ret);

        if(size > 0) {
            if(size <= std::numeric_limits<int8_t>().max())
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64r8i, m_registerInfo->getRegister(x64::RegisterId::RSP), ctx->getImmediateInt(size, MIR::ImmediateInt::imm8)), idx++);
            else
                bb->addInstructionAt(instr((uint32_t)Opcode::Add64r32i, m_registerInfo->getRegister(x64::RegisterId::RSP), ctx->getImmediateInt(size, MIR::ImmediateInt::imm32)), idx++);

        }
        bb->addInstructionAt(instr((uint32_t)Opcode::Pop64r, m_registerInfo->getRegister(x64::RegisterId::RBP)), idx++);
    }

    m_returnInstructions.clear();
}

void x64TargetLowering::lowerSwitch(MIR::Block* block, MIR::SwitchLowering* lowering) {
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
    IR::GlobalVariable* var = IR::GlobalVariable::get(*unit, voidPtr, array, IR::Linkage::Internal);

    MIR::GlobalAddress* addr = var->getMachineGlobalAddress(*unit);
    x64InstructionInfo* xInstrInfo = (x64InstructionInfo*)m_instructionInfo;
    // MIR::Register* rr = m_registerInfo->getRegister(m_registerInfo->getReservedRegisters(GPR64).back());
    MIR::Register* rr = xInstrInfo->getRegisterInfo()->getRegister(
        block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(GPR64)
    );
    block->addInstructionAt(xInstrInfo->memoryToOperand((uint32_t)Opcode::Lea64rm, rr, m_registerInfo->getRegister(RIP), 0, nullptr, 1, addr), inIdx++);
    MIR::Operand* index = instruction->getCondition();
    if(min != 0 || index->isImmediateInt()) {
        MIR::Register* tmp = m_registerInfo->getRegister(
            block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(GPR64)
        );
        index = block->getParentFunction()->cloneOpWithFlags(index, Force64BitRegister);
        inIdx += xInstrInfo->move(block, inIdx, index, tmp, 8, false);
        index = tmp;
    }
    else if(index->isRegister()) {
        index = block->getParentFunction()->cloneOpWithFlags(index, Force64BitRegister);
    }

    block->addInstructionAt(instr((uint32_t)Opcode::Cmp64r32i, index, unit->getContext()->getImmediateInt(min, MIR::ImmediateInt::imm32)), inIdx++);
    block->addInstructionAt(instr((uint32_t)Opcode::Jl, instruction->getDefault()), inIdx++);
    block->addInstructionAt(instr((uint32_t)Opcode::Cmp64r32i, index, unit->getContext()->getImmediateInt(max, MIR::ImmediateInt::imm32)), inIdx++);
    block->addInstructionAt(instr((uint32_t)Opcode::Jg, instruction->getDefault()), inIdx++);

    if(min != 0) {
        block->addInstructionAt(instr((uint32_t)Opcode::Sub64r32i, index, unit->getContext()->getImmediateInt(min, MIR::ImmediateInt::imm32)), inIdx++);
    }

    block->addInstructionAt(
        xInstrInfo->memoryToOperand((uint32_t)Opcode::Mov64rm, rr, rr, 0, cast<MIR::Register>(index), 8, nullptr)
    , inIdx++);
    block->addInstructionAt(instr((uint32_t)Opcode::Jmp64r, rr), inIdx++);
}

void x64TargetLowering::lowerReturn(MIR::Block* block, MIR::ReturnLowering* lowering) {
    size_t inIdx = block->getInstructionIdx(lowering);
    std::unique_ptr<MIR::ReturnLowering> instruction = std::unique_ptr<MIR::ReturnLowering>(
        cast<MIR::ReturnLowering>(block->removeInstruction(lowering).release())
    );

    CallInfo info(m_registerInfo, m_dataLayout);
    CallConvFunction ccfunc = m_os == OS::Linux ? CCx64SysV : m_os == OS::Windows ? CCx64Win64 : nullptr;
    info.analyzeFormalArgs(ccfunc, lowering->getParentBlock()->getParentFunction());

    for(size_t i = 0; i < info.getRetAssigns().size(); i++) {
        auto& ret = info.getRetAssigns().at(i);
        if(auto regRet = dyn_cast<RegisterAssign>(ret)) {
            uint32_t classid = m_registerInfo->getRegisterIdClass(regRet->getRegister(), block->getParentFunction()->getRegisterInfo());
            if(lowering->getValue()->isRegister() || lowering->getValue()->isImmediateInt()) {
                inIdx += m_instructionInfo->move(block, inIdx, lowering->getValue(), m_registerInfo->getRegister(regRet->getRegister()), regRet->getSize(), classid == FPR);
            }
            else if(lowering->getValue()->isMultiValue()) {
                MIR::MultiValue* multi = cast<MIR::MultiValue>(lowering->getValue());
                inIdx += m_instructionInfo->move(block, inIdx, multi->getValues().at(i), m_registerInfo->getRegister(regRet->getRegister()), regRet->getSize(), classid == FPR);
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

void x64TargetLowering::parallelCopy(MIR::Block* block) {
    auto& copies = block->getPhiLowering();
    MIR::Instruction* term = block->getTerminator(m_instructionInfo);

    UMap<MIR::Operand*, MIR::Operand*> m;
    for (auto& [dest, src] : copies)
        m[dest] = src;

    USet<MIR::Operand*> visited;
    MIR::Register* tmp = nullptr;

    for (auto& [dest, src] : m) {
        if (dest == src) continue;
        if (visited.count(dest)) continue;

        MIR::Operand* d = dest;
        MIR::Operand* s = src;

        std::vector<MIR::Operand*> cycle;
        while (m.count(s) && m[s] != d && !visited.count(s)) {
            cycle.push_back(s);
            s = m[s];
        }

        uint32_t classid = 0;
        if(s->isRegister()) {
            classid = m_registerInfo->getRegisterIdClass(cast<MIR::Register>(s)->getId(), block->getParentFunction()->getRegisterInfo());
        }
        else if(s->isImmediateInt()) {
            MIR::ImmediateInt* imm = cast<MIR::ImmediateInt>(s);
            switch(imm->getSize()) {
                case 8: classid = GPR64; break;
                case 4: classid = GPR32; break;
                case 2: classid = GPR16; break;
                case 1: classid = GPR8; break;
            }
        }
        else throw std::runtime_error("Not implemented");
        auto rclass = m_registerInfo->getRegisterClass(classid);

        if (s == d) {
            tmp = m_registerInfo->getRegister(block->getParentFunction()->getRegisterInfo().getNextVirtualRegister(classid));
            m_instructionInfo->move(block, block->getInstructionIdx(term), s, tmp, rclass.getSize(), classid == FPR);

            m[cycle.back()] = tmp;
        }

        s = m[d];
        m_instructionInfo->move(block, block->getInstructionIdx(term), s, d, rclass.getSize(), classid == FPR);
        visited.insert(d);
    }
}

}