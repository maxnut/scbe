#pragma once

#include "MIR/block.hpp"
#include "MIR/instruction.hpp"
#include "MIR/operand.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
namespace scbe::Target::x64 {

enum class Opcode : uint32_t {
    Mov64rr = 0, Movr64i64, Movr64i32, Mov64mr, Mov64rm, Movm64i32,
    Mov32rr, Mov32ri, Mov32mr, Mov32rm, Mov32mi,
    Mov16rr, Mov16ri, Mov16mr, Mov16rm, Mov16mi,
    Mov8rr, Mov8ri, Mov8mr, Mov8rm, Mov8mi,
    Add64rr, Add64rm, Add64mr,
    Sub64rr, Sub64rm, Sub64mr,
    Add32rr, Add32ri, Add32mi, Add32rm, Add32mr,
    Sub32rr, Sub32ri, Sub32mi, Sub32rm, Sub32mr,
    Add16rr, Add16ri, Add16mi, Add16rm, Add16mr,
    Sub16rr, Sub16ri, Sub16mi, Sub16rm, Sub16mr,
    Add8rr, Add8ri, Add8mi, Add8rm, Add8mr,
    Sub8rr, Sub8ri, Sub8mi, Sub8rm, Sub8mr,

    Add64r32i, Add64r8i,
    Sub64r32i, Sub64r8i,
    
    Ret,
    Push64r, Pop64r,
    Push32r, Pop32r,
    Push16r, Pop16r,
    Cmp64rr, Cmp64rm, Cmp64r32i,
    Cmp32rr, Cmp32ri, Cmp32rm, Cmp32mi,
    Cmp16rr, Cmp16ri, Cmp16rm, Cmp16mi,
    Cmp8rr, Cmp8ri, Cmp8rm, Cmp8mi,
    Test64rr, Test32rr, Test16rr, Test8rr,
    Jmp,
    Je, Jne, Jl, Jle, Jg, Jge,
    Jb, Jbe, Ja, Jae,

    Jmp64r,

    Movssrr, Movssrm, Movssmr,
    Movsdrr, Movsdrm, Movsdmr,

    Addssrr, Addssrm, Addssmr,
    Subssrr, Subssrm, Subssmr,
    Mulssrr,
    
    Addsdrr, Addsdrm, Addsdmr,
    Subsdrr, Subsdrm, Subsdmr,
    Mulsdrr,

    Lea64rm,
    Lea32rm,
    Lea16rm,

    Call, Call64r,

    Rep_Movsb,

    Movzx64r16r, Movzx64r16m,
    Movzx32r16r, Movzx32r16m,
    Movzx64r8r, Movzx64r8m,
    Movzx32r8r, Movzx32r8m,
    Movzx16r8r, Movzx16r8m,

    Movsx64r32r, Movsx64r32m,
    Movsx32r32r, Movsx32r32m,
    Movsx64r16r, Movsx64r16m,
    Movsx32r16r, Movsx32r16m,
    Movsx64r8r, Movsx64r8m,
    Movsx32r8r, Movsx32r8m,
    Movsx16r8r, Movsx16r8m,

    Cvtss2sdrr, Cvtss2sdrm,
    Cvtsd2ssrr, Cvtsd2ssrm,
    Cvtsi2ss64rr, Cvtsi2ss64rm,
    Cvtsi2ss32rr, Cvtsi2ss32rm,
    Cvtsi2sd64rr, Cvtsi2sd64rm,
    Cvtsi2sd32rr, Cvtsi2sd32rm,
    Cvtsd2si64rr, Cvtsd2si64rm,
    Cvtsd2si32rr, Cvtsd2si32rm,
    Cvtss2si64rr, Cvtss2si64rm,
    Cvtss2si32rr, Cvtss2si32rm,

    Shl8ri, Shl8rCL,
    Shl16ri, Shl16rCL,
    Shl32ri, Shl32rCL,
    Shl64ri, Shl64rCL,

    Shr8ri, Shr8rCL,
    Shr16ri, Shr16rCL,
    Shr32ri, Shr32rCL,
    Shr64ri, Shr64rCL,

    Sar8ri, Sar8rCL,
    Sar16ri, Sar16rCL,
    Sar32ri, Sar32rCL,
    Sar64ri, Sar64rCL,

    And8ri, And8rr,
    And16ri, And16rr,
    And32ri, And32rr,
    And64rr,

    Or8ri, Or8rr,
    Or16ri, Or16rr,
    Or32ri, Or32rr,
    Or64rr,

    Xor8ri, Xor8rr,
    Xor16ri, Xor16rr,
    Xor32ri, Xor32rr,
    Xor64rr,

    Not64r, Not32r, Not16r, Not8r,

    Cmpssrr,
    Cmpsdrr,

    Ucomissrr,
    Ucomisdrr,

    Sete, Setne, Setl, Setle, Setg, Setge,
    Setb, Setbe, Seta, Setae,

    Cqo, Cdq, Cwd, Cbw,

    IDiv64, IDiv32, IDiv16, IDiv8,
    Div64, Div32, Div16, Div8,
    Divssrr, Divsdrr,

    Mul64, Mul32, Mul16, Mul8,
    IMul64rr, IMul32rr, IMul16rr,
    IMul64rri, IMul32rri, IMul16rri,

    Movapsrm, Movapsmr, Movapsrr,
    
    Push32i, Push16i, Push8i,

    Count
};

class x64InstructionInfo : public InstructionInfo {
public:
    x64InstructionInfo(RegisterInfo* registerInfo, Ref<Context> ctx);

    size_t registerToStackSlot(MIR::Block* block, size_t pos, MIR::Register* reg, MIR::StackSlot stackSlot) override;
    size_t stackSlotToRegister(MIR::Block* block, size_t pos, MIR::Register* reg, MIR::StackSlot stackSlot) override;
    size_t immediateToStackSlot(MIR::Block* block, size_t pos, MIR::ImmediateInt* immediate, MIR::StackSlot stackSlot) override;
    size_t move(MIR::Block* block, size_t pos, MIR::Operand* source, MIR::Operand* destination, size_t size, bool flt) override;

    size_t stackSlotAddress(MIR::Block* block, size_t pos, MIR::StackSlot stackSlot, MIR::Register* destination);

    std::unique_ptr<MIR::Instruction> operandToMemory(uint32_t op, MIR::Operand* operand, MIR::Register* base, int64_t offset, MIR::Register* index = nullptr, size_t scale = 1, MIR::Symbol* symbol = nullptr);
    std::unique_ptr<MIR::Instruction> memoryToOperand(uint32_t op, MIR::Operand* operand, MIR::Register* base, int64_t offset, MIR::Register* index = nullptr, size_t scale = 1, MIR::Symbol* symbol = nullptr);
};

}