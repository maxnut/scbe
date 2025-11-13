#include "target/x64/x64_instruction_info.hpp"
#include "ISel/DAG/node.hpp"
#include "ISel/DAG/pattern.hpp"
#include "MIR/function.hpp"
#include "MIR/operand.hpp"
#include "MIR/stack_slot.hpp"
#include "cast.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include "target/x64/x64_register_info.hpp"
#include "target/instruction_utils.hpp"
#include "target/x64/x64_patterns.hpp"

#include <cstdint>

namespace scbe::Target::x64 {

using namespace ISel::DAG;

#define MEMORY_RESTRICTION Restriction::reg(), Restriction::imm(), Restriction::reg(), Restriction::imm(), Restriction::sym() 

x64InstructionInfo::x64InstructionInfo(RegisterInfo* registerInfo, Ref<Context> ctx) : InstructionInfo(registerInfo, ctx) {
    m_instructionDescriptors = []{
        std::vector<InstructionDescriptor> ret((size_t)Opcode::Count);
        // name, numDefs, numOps, size, mayStore, mayLoad, restrictions
        ret[(size_t)Opcode::Mov64rr] = {"Mov64rr"    , 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movr64i64] = {"Movr64i64"    , 1, 2, 8, false, false, {Restriction::reg(true), Restriction::imm()}};
        ret[(size_t)Opcode::Movr64i32] = {"Movr64i32"    , 1, 2, 8, false, false, {Restriction::reg(true), Restriction::imm()}};
        ret[(size_t)Opcode::Mov64mr] = {"Mov64mr"    , 0, 6, 8, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Mov64rm] = {"Mov64rm"    , 1, 6, 8, false, true , {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movm64i32] = {"Movm64i32"    , 0, 6, 8, true , false, {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Mov32rr] = {"Mov32rr"    , 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Mov32ri] = {"Mov32ri"    , 1, 2, 4, false, false, {Restriction::reg(true), Restriction::imm()}};
        ret[(size_t)Opcode::Mov32mr] = {"Mov32mr"    , 0, 6, 4, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Mov32rm] = {"Mov32rm"    , 1, 6, 4, false, true , {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Mov32mi] = {"Mov32mi"    , 0, 6, 4, true , false, {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Mov16rr] = {"Mov16rr"    , 1, 2, 2, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Mov16ri] = {"Mov16ri"    , 1, 2, 2, false, false, {Restriction::reg(true), Restriction::imm()}};
        ret[(size_t)Opcode::Mov16mr] = {"Mov16mr"    , 0, 6, 2, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Mov16rm] = {"Mov16rm"    , 1, 6, 2, false, true , {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Mov16mi] = {"Mov16mi"    , 0, 6, 2, true , false, {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Mov8rr] = {"Mov8rr"     , 1, 2, 1, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Mov8ri] = {"Mov8ri"     , 1, 2, 1, false, false, {Restriction::reg(true), Restriction::imm()}};
        ret[(size_t)Opcode::Mov8mr] = {"Mov8mr"     , 0, 6, 1, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Mov8rm] = {"Mov8rm"     , 1, 6, 1, false, true , {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Mov8mi] = {"Mov8mi"     , 0, 6, 1, true , false, {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Add64rr] = {"Add64rr"    , 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Add64rm] = {"Add64rm"    , 1, 6, 8, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Add64mr] = {"Add64mr"    , 0, 6, 8, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};

        ret[(size_t)Opcode::Sub64rr] = {"Sub64rr"    , 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sub64rm] = {"Sub64rm"    , 1, 6, 8, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Sub64mr] = {"Sub64mr"    , 0, 6, 8, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};

        ret[(size_t)Opcode::Add32rr] = {"Add32rr"    , 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Add32ri] = {"Add32ri"    , 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Add32mi] = {"Add32mi"    , 0, 6, 4, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};
        ret[(size_t)Opcode::Add32rm] = {"Add32rm"    , 1, 6, 4, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Add32mr] = {"Add32mr"    , 0, 6, 4, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
                              
        ret[(size_t)Opcode::Sub32rr] = {"Sub32rr"    , 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sub32ri] = {"Sub32ri"    , 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sub32mi] = {"Sub32mi"    , 0, 6, 4, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};
        ret[(size_t)Opcode::Sub32rm] = {"Sub32rm"    , 1, 6, 4, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Sub32mr] = {"Sub32mr"    , 0, 6, 4, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
                              
        ret[(size_t)Opcode::Add16rr] = {"Add16rr"    , 1, 2, 2, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Add16ri] = {"Add16ri"    , 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Add16mi] = {"Add16mi"    , 0, 6, 2, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};
        ret[(size_t)Opcode::Add16rm] = {"Add16rm"    , 1, 6, 2, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Add16mr] = {"Add16mr"    , 0, 6, 2, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
                              
        ret[(size_t)Opcode::Sub16rr] = {"Sub16rr"    , 1, 2, 2, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sub16ri] = {"Sub16ri"    , 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sub16mi] = {"Sub16mi"    , 0, 6, 2, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};
        ret[(size_t)Opcode::Sub16rm] = {"Sub16rm"    , 1, 6, 2, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Sub16mr] = {"Sub16mr"    , 0, 6, 2, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
                             
        ret[(size_t)Opcode::Add8rr] = {"Add8rr"     , 1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Add8ri] = {"Add8ri"     , 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Add8mi] = {"Add8mi"     , 0, 6, 1, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};
        ret[(size_t)Opcode::Add8rm] = {"Add8rm"     , 1, 6, 1, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Add8mr] = {"Add8mr"     , 0, 6, 1, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
                              
        ret[(size_t)Opcode::Sub8rr] = {"Sub8rr"     , 1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sub8ri] = {"Sub8ri"     , 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sub8mi] = {"Sub8mi"     , 0, 6, 1, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};
        ret[(size_t)Opcode::Sub8rm] = {"Sub8rm"     , 1, 6, 1, false, true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Sub8mr] = {"Sub8mr"     , 0, 6, 1, true , false, {MEMORY_RESTRICTION, Restriction::reg()}};
                             
        ret[(size_t)Opcode::Add64r32i] = {"Add64r32i" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Add64r8i] = {"Add64r8i" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::imm()}};

        ret[(size_t)Opcode::Sub64r32i] = {"Sub64r8i" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sub64r8i] = {"Sub64r8i" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::imm()}};

        ret[(size_t)Opcode::Ret] = {"Ret"        , 0, 0, 0, false, false, {}, {}, true};

        ret[(size_t)Opcode::Push64r] = {"Push64r"        , 0, 1, 8, false, false, {Restriction::reg()} };
        ret[(size_t)Opcode::Pop64r] = {"Pop64r"        , 0, 1, 8, false, false, {Restriction::reg()} };

        ret[(size_t)Opcode::Push32r] = {"Push32r"        , 0, 1, 4, false, false, {Restriction::reg()} };
        ret[(size_t)Opcode::Pop32r] = {"Pop32r"        , 0, 1, 4, false, false, {Restriction::reg()} };

        ret[(size_t)Opcode::Push16r] = {"Push16r"        , 0, 1, 2, false, false, {Restriction::reg()} };
        ret[(size_t)Opcode::Pop16r] = {"Pop16r"        , 0, 1, 2, false, false, {Restriction::reg()} };

        ret[(size_t)Opcode::Cmp64rr] = {"Cmp64rr"     , 0, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Cmp64rm] = {"Cmp64rm"     , 0, 6, 8, false , true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cmp64r32i] = {"Cmp64r32i"     , 0, 2, 8, false , false , {Restriction::reg(), Restriction::imm()}};

        ret[(size_t)Opcode::Cmp32rr] = {"Cmp32rr"     , 0, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Cmp32ri] = {"Cmp32ri"     , 0, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Cmp32rm] = {"Cmp32rm"     , 0, 6, 4, false , true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cmp32mi] = {"Cmp32mi"     , 0, 6, 4, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Cmp16rr] = {"Cmp16rr"     , 0, 2, 2, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Cmp16ri] = {"Cmp16ri"     , 0, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Cmp16rm] = {"Cmp16rm"     , 0, 6, 2, false , true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cmp16mi] = {"Cmp16mi"     , 0, 6, 2, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Cmp8rr] = {"Cmp8rr"     , 0, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Cmp8ri] = {"Cmp8ri"     , 0, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Cmp8rm] = {"Cmp8rm"     , 0, 6, 1, false , true , {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cmp8mi] = {"Cmp8mi"     , 0, 6, 1, true , false , {MEMORY_RESTRICTION, Restriction::imm()}};

        ret[(size_t)Opcode::Jmp] = {"Jmp"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Je] = {"Je"         , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jne] = {"Jne"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jl] = {"Jl"         , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jle] = {"Jle"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jg] = {"Jg"         , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jge] = {"Jge"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jb] = {"Jb"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jbe] = {"Jbe"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Ja] = {"Ja"        , 0, 1, 8, false, false, {Restriction::sym()}};
        ret[(size_t)Opcode::Jae] = {"Jae"        , 0, 1, 8, false, false, {Restriction::sym()}};

        ret[(size_t)Opcode::Jmp64r] = {"Jmp64r"        , 0, 1, 8, false, false, {Restriction::reg()}};

        ret[(size_t)Opcode::Movssrr] = {"Movssrr"    , 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movssrm] = {"Movssrm"    , 1, 6, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movssmr] = {"Movssmr"    , 0, 6, 4, true, false, {MEMORY_RESTRICTION, Restriction::reg()}};

        ret[(size_t)Opcode::Movsdrr] = {"Movsdrr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsdrm] = {"Movsdrm", 1, 6, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsdmr] = {"Movsdmr", 0, 6, 8, true, false, {MEMORY_RESTRICTION, Restriction::reg()}};

        ret[(size_t)Opcode::Addssrr] = {"Addssrr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Addssrm] = {"Addssrm", 1, 6, 4, false, true, {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Addssmr] = {"Addssmr", 0, 6, 4, true, false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Subssrr] = {"Subssrr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Subssrm] = {"Subssrm", 1, 6, 4, false, true, {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Subssmr] = {"Subssmr", 0, 6, 4, true, false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Mulssrr] = {"Mulssrr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Addsdrr] = {"Addsdrr" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Addsdrm] = {"Addsdrm" , 1, 6, 8, false, true, {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Addsdmr] = {"Addsdmr" , 0, 6, 8, true, false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Subsdrr] = {"Subsdrr" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Subsdrm] = {"Subsdrm" , 1, 6, 8, false, true, {Restriction::reg(), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Subsdmr] = {"Subsdmr" , 0, 6, 8, true, false, {MEMORY_RESTRICTION, Restriction::reg()}};
        ret[(size_t)Opcode::Mulsdrr] = {"Mulsdrr" , 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        
        ret[(size_t)Opcode::Lea64rm] = {"Lea64rm", 1, 6, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Lea32rm] = {"Lea32rm", 1, 6, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Lea16rm] = {"Lea16rm", 1, 6, 2, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};

        ret[(size_t)Opcode::Call] = {"Call", 0, 1, 8, false, false, {Restriction({(uint32_t)MIR::Operand::Kind::GlobalAddress, (uint32_t)MIR::Operand::Kind::ExternalSymbol, (uint32_t)MIR::Operand::Kind::Register}, false)}};
        ret[(size_t)Opcode::Call64r] = {"Call64r", 0, 1, 8, false, false, {Restriction::reg()}};
        
        ret[(size_t)Opcode::Rep_Movsb] = {"Rep_Movsb", 0, 0, 0, false, false, {}};

        ret[(size_t)Opcode::Movzx64r16r] = {"Movzx64r16r", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movzx64r16m] = {"Movzx64r16m", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movzx32r16r] = {"Movzx32r16r", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movzx32r16m] = {"Movzx32r16m", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movzx64r8r] = {"Movzx64r8r", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movzx64r8m] = {"Movzx64r8m", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movzx32r8r] = {"Movzx32r8r", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movzx32r8m] = {"Movzx32r8m", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movzx16r8r] = {"Movzx16r8r", 1, 2, 2, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movzx16r8m] = {"Movzx16r8m", 1, 2, 2, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx64r32r] = {"Movsx64r32r", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx64r32m] = {"Movsx64r32m", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx32r32r] = {"Movsx32r32r", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx32r32m] = {"Movsx32r32m", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx64r16r] = {"Movsx64r16r", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx64r16m] = {"Movsx64r16m", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx32r16r] = {"Movsx32r16r", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx32r16m] = {"Movsx32r16m", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx64r8r] = {"Movsx64r8r", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx64r8m] = {"Movsx64r8m", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx32r8r] = {"Movsx32r8r", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx32r8m] = {"Movsx32r8m", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Movsx16r8r] = {"Movsx16r8r", 1, 2, 2, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Movsx16r8m] = {"Movsx16r8m", 1, 2, 2, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtss2sdrr] = {"Cvtss2sdrr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtss2sdrm] = {"Cvtss2sdrm", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsd2ssrr] = {"Cvtsd2ssrr", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsd2ssrm] = {"Cvtsd2ssrm", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsi2ss64rr] = {"Cvtsi2ss64rr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsi2ss64rm] = {"Cvtsi2ss64rm", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsi2ss32rr] = {"Cvtsi2ss32rr", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsi2ss32rm] = {"Cvtsi2ss32rm", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsi2sd64rr] = {"Cvtsi2sd64rr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsi2sd64rm] = {"Cvtsi2sd64rm", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsi2sd32rr] = {"Cvtsi2sd32rr", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsi2sd32rm] = {"Cvtsi2sd32rm", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsd2si64rr] = {"Cvtsd2si64rr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsd2si64rm] = {"Cvtsd2si64rm", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtsd2si32rr] = {"Cvtsd2si32rr", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtsd2si32rm] = {"Cvtsd2si32rm", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtss2si64rr] = {"Cvtss2si64rr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtss2si64rm] = {"Cvtss2si64rm", 1, 2, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};
        ret[(size_t)Opcode::Cvtss2si32rr] = {"Cvtss2si32rr", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()}};
        ret[(size_t)Opcode::Cvtss2si32rm] = {"Cvtss2si32rm", 1, 2, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION}};

        ret[(size_t)Opcode::Shl8ri] = {"Shl8ri", 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shl16ri] = {"Shl8ri", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shl32ri] = {"Shl8ri", 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shl64ri] = {"Shl8ri", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Shr8ri] = {"Shr8ri", 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shr16ri] = {"Shr8ri", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shr32ri] = {"Shr8ri", 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shr64ri] = {"Shr8ri", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Shr64rCL] = {"Shr8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Sar8ri] = {"Sar8ri", 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sar8rCL] = {"Sar8rCL", 1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sar16ri] = {"Sar8ri", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sar16rCL] = {"Sar8rCL", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sar32ri] = {"Sar8ri", 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Sar32rCL] = {"Sar8rCL", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sar64ri] = {"Sar8ri", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Sar64rCL] = {"Sar8rCL", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::And8ri] = {"And8ri", 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::And8rr] = {"And8rr", 1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::And16ri] = {"And8ri", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::And16rr] = {"And8rr", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::And32ri] = {"And8ri", 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::And32rr] = {"And8rr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::And64rr] = {"And8rr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Or8ri] = {"Or8ri", 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Or8rr] = {"Or8rr", 1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Or16ri] = {"Or8ri", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Or16rr] = {"Or8rr", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Or32ri] = {"Or8ri", 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Or32rr] = {"Or8rr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Or64rr] = {"Or8rr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Xor8ri] = {"Xor8ri", 1, 2, 1, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Xor8rr] = {"Xor8rr", 1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Xor16ri] = {"Xor8ri", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Xor16rr] = {"Xor8rr", 1, 2, 2, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Xor32ri] = {"Xor8ri", 1, 2, 4, false, false, {Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Xor32rr] = {"Xor8rr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Xor64rr] = {"Xor8rr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Cmpsdrr] = {"Cmpsdrr", 1, 3, 8, false, false, {Restriction::reg(), Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::Cmpssrr] = {"Cmpssrr", 1, 3, 4, false, false, {Restriction::reg(), Restriction::reg(), Restriction::imm()}};

        ret[(size_t)Opcode::Ucomisdrr] = {"Ucomisdrr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Ucomissrr] = {"Ucomissrr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Sete] = {"Sete", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setne] = {"Setne", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setl] = {"Setl", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setle] = {"Setle", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setg] = {"Setg", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setge] = {"Setge", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setb] = {"Setb", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setbe] = {"Setbe", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Seta] = {"Seta", 1, 1, 8, false, false, {Restriction::reg(true)}};
        ret[(size_t)Opcode::Setae] = {"Setae", 1, 1, 8, false, false, {Restriction::reg(true)}};

        ret[(size_t)Opcode::Cqo] = {"Cqo", 0, 0, 0, false, false, {}};
        ret[(size_t)Opcode::Cdq] = {"Cdq", 0, 0, 0, false, false, {}};
        ret[(size_t)Opcode::Cwd] = {"Cwd", 0, 0, 0, false, false, {}};
        ret[(size_t)Opcode::Cbw] = {"Cbw", 0, 0, 0, false, false, {}};

        ret[(size_t)Opcode::IDiv64] = {"IDiv64", 1, 1, 8, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::IDiv32] = {"IDiv32", 1, 1, 4, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::IDiv16] = {"IDiv16", 1, 1, 2, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::IDiv8] = {"IDiv8", 1, 1, 1, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Div64] = {"Div64", 1, 1, 8, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Div32] = {"Div32", 1, 1, 4, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Div16] = {"Div16", 1, 1, 2, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Div8] = {"Div8", 1, 1, 1, false, false, {Restriction::reg()}, {RDX, RAX}};

        ret[(size_t)Opcode::Divsdrr] = {"Divsdrr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Divssrr] = {"Divssrr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Mul64] = {"Mul64"    , 1, 1, 8, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Mul32] = {"Mul32"    , 1, 1, 4, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Mul16] = {"Mul16"    , 1, 1, 2, false, false, {Restriction::reg()}, {RDX, RAX}};
        ret[(size_t)Opcode::Mul8] = {"Mul8"    , 1, 1, 1, false, false, {Restriction::reg()}, {RDX, RAX}};

        ret[(size_t)Opcode::IMul64rr] = {"IMul64rr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::IMul32rr] = {"IMul32rr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::IMul16rr] = {"IMul16rr", 1, 2, 2, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::IMul64rri] = {"IMul64rri", 1, 3, 8, false, false, {Restriction::reg(), Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::IMul32rri] = {"IMul32rri", 1, 3, 4, false, false, {Restriction::reg(), Restriction::reg(), Restriction::imm()}};
        ret[(size_t)Opcode::IMul16rri] = {"IMul16rri", 1, 3, 2, false, false, {Restriction::reg(), Restriction::reg(), Restriction::imm()}};

        ret[(size_t)Opcode::Test64rr] = {"Test64rr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Test32rr] = {"Test32rr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Test16rr] = {"Test16rr", 1, 2, 2, false, false, {Restriction::reg(), Restriction::reg()}};
        ret[(size_t)Opcode::Test8rr] = {"Test8rr",  1, 2, 1, false, false, {Restriction::reg(), Restriction::reg()}};

        ret[(size_t)Opcode::Not64r] = {"Not64r", 1, 1, 8, false, false, {Restriction::reg()}};
        ret[(size_t)Opcode::Not32r] = {"Not32r", 1, 1, 8, false, false, {Restriction::reg()}};
        ret[(size_t)Opcode::Not16r] = {"Not16r", 1, 1, 8, false, false, {Restriction::reg()}};
        ret[(size_t)Opcode::Not8r] = {"Not8r", 1, 1, 8, false, false, {Restriction::reg()}};

        return ret;
    }();
    
    m_mnemonics = [] {
        std::vector<Mnemonic> ret((size_t)Opcode::Count);
        ret[(size_t)Opcode::Mov64rr] = {"mov"};
        ret[(size_t)Opcode::Movr64i64] = {"movabs"};
        ret[(size_t)Opcode::Movr64i32] = {"mov"};
        ret[(size_t)Opcode::Mov64mr] = {"mov"};
        ret[(size_t)Opcode::Mov64rm] = {"mov"};
        ret[(size_t)Opcode::Movm64i32] = {"mov"};

        ret[(size_t)Opcode::Mov32rr] = {"mov"};
        ret[(size_t)Opcode::Mov32ri] = {"mov"};
        ret[(size_t)Opcode::Mov32mr] = {"mov"};
        ret[(size_t)Opcode::Mov32rm] = {"mov"};
        ret[(size_t)Opcode::Mov32mi] = {"mov"};

        ret[(size_t)Opcode::Mov16rr] = {"mov"};
        ret[(size_t)Opcode::Mov16ri] = {"mov"};
        ret[(size_t)Opcode::Mov16mr] = {"mov"};
        ret[(size_t)Opcode::Mov16rm] = {"mov"};
        ret[(size_t)Opcode::Mov16mi] = {"mov"};

        ret[(size_t)Opcode::Mov8rr] = {"mov"};
        ret[(size_t)Opcode::Mov8ri] = {"mov"};
        ret[(size_t)Opcode::Mov8mr] = {"mov"};
        ret[(size_t)Opcode::Mov8rm] = {"mov"};
        ret[(size_t)Opcode::Mov8mi] = {"mov"};

        ret[(size_t)Opcode::Add64rr] = {"add"};
        ret[(size_t)Opcode::Add64rm] = {"add"};
        ret[(size_t)Opcode::Add64mr] = {"add"};

        ret[(size_t)Opcode::Sub64rr] = {"sub"};
        ret[(size_t)Opcode::Sub64rm] = {"sub"};
        ret[(size_t)Opcode::Sub64mr] = {"sub"};

        ret[(size_t)Opcode::Add32rr] = {"add"};
        ret[(size_t)Opcode::Add32ri] = {"add"};
        ret[(size_t)Opcode::Add32mi] = {"add"};
        ret[(size_t)Opcode::Add32rm] = {"add"};
        ret[(size_t)Opcode::Add32mr] = {"add"};
                              
        ret[(size_t)Opcode::Sub32rr] = {"sub"};
        ret[(size_t)Opcode::Sub32ri] = {"sub"};
        ret[(size_t)Opcode::Sub32mi] = {"sub"};
        ret[(size_t)Opcode::Sub32rm] = {"sub"};
        ret[(size_t)Opcode::Sub32mr] = {"sub"};
                              
        ret[(size_t)Opcode::Add16rr] = {"add"};
        ret[(size_t)Opcode::Add16ri] = {"add"};
        ret[(size_t)Opcode::Add16mi] = {"add"};
        ret[(size_t)Opcode::Add16rm] = {"add"};
        ret[(size_t)Opcode::Add16mr] = {"add"};
                              
        ret[(size_t)Opcode::Sub16rr] = {"sub"};
        ret[(size_t)Opcode::Sub16ri] = {"sub"};
        ret[(size_t)Opcode::Sub16mi] = {"sub"};
        ret[(size_t)Opcode::Sub16rm] = {"sub"};
        ret[(size_t)Opcode::Sub16mr] = {"sub"};
                             
        ret[(size_t)Opcode::Add8rr] = {"add"};
        ret[(size_t)Opcode::Add8ri] = {"add"};
        ret[(size_t)Opcode::Add8mi] = {"add"};
        ret[(size_t)Opcode::Add8rm] = {"add"};
        ret[(size_t)Opcode::Add8mr] = {"add"};
                              
        ret[(size_t)Opcode::Sub8rr] = {"sub"};
        ret[(size_t)Opcode::Sub8ri] = {"sub"};
        ret[(size_t)Opcode::Sub8mi] = {"sub"};
        ret[(size_t)Opcode::Sub8rm] = {"sub"};
        ret[(size_t)Opcode::Sub8mr] = {"sub"};
                             
        ret[(size_t)Opcode::Add64r32i] = {"add"};
        ret[(size_t)Opcode::Add64r8i] = {"add"};

        ret[(size_t)Opcode::Sub64r32i] = {"sub"};
        ret[(size_t)Opcode::Sub64r8i] = {"sub"};

        ret[(size_t)Opcode::Ret] = {"ret"};

        ret[(size_t)Opcode::Push64r] = {"push"};
        ret[(size_t)Opcode::Pop64r] = {"pop"};

        ret[(size_t)Opcode::Push32r] = {"push"};
        ret[(size_t)Opcode::Pop32r] = {"pop"};

        ret[(size_t)Opcode::Push16r] = {"push"};
        ret[(size_t)Opcode::Pop16r] = {"pop"};

        ret[(size_t)Opcode::Cmp64rr] = {"cmp"};
        ret[(size_t)Opcode::Cmp64rm] = {"cmp"};
        ret[(size_t)Opcode::Cmp64r32i] = {"cmp"};

        ret[(size_t)Opcode::Cmp32rr] = {"cmp"};
        ret[(size_t)Opcode::Cmp32ri] = {"cmp"};
        ret[(size_t)Opcode::Cmp32rm] = {"cmp"};
        ret[(size_t)Opcode::Cmp32mi] = {"cmp"};

        ret[(size_t)Opcode::Cmp16rr] = {"cmp"};
        ret[(size_t)Opcode::Cmp16ri] = {"cmp"};
        ret[(size_t)Opcode::Cmp16rm] = {"cmp"};
        ret[(size_t)Opcode::Cmp16mi] = {"cmp"};

        ret[(size_t)Opcode::Cmp8rr] = {"cmp"};
        ret[(size_t)Opcode::Cmp8ri] = {"cmp"};
        ret[(size_t)Opcode::Cmp8rm] = {"cmp"};
        ret[(size_t)Opcode::Cmp8mi] = {"cmp"};

        ret[(size_t)Opcode::Jmp] = {"jmp"};
        ret[(size_t)Opcode::Je] = {"je"};
        ret[(size_t)Opcode::Jne] = {"jne"};
        ret[(size_t)Opcode::Jl] = {"jl"};
        ret[(size_t)Opcode::Jle] = {"jle"};
        ret[(size_t)Opcode::Jg] = {"jg"};
        ret[(size_t)Opcode::Jge] = {"jge"};
        ret[(size_t)Opcode::Jb] = {"jb"};
        ret[(size_t)Opcode::Jbe] = {"jbe"};
        ret[(size_t)Opcode::Ja] = {"ja"};
        ret[(size_t)Opcode::Jae] = {"jae"};

        ret[(size_t)Opcode::Jmp64r] = {"jmp"};

        ret[(size_t)Opcode::Movssrr] = {"movss"};
        ret[(size_t)Opcode::Movssrm] = {"movss"};
        ret[(size_t)Opcode::Movssmr] = {"movss"};

        ret[(size_t)Opcode::Movsdrr] = {"movsd"};
        ret[(size_t)Opcode::Movsdrm] = {"movsd"};
        ret[(size_t)Opcode::Movsdmr] = {"movsd"};

        ret[(size_t)Opcode::Addssrr] = {"addss"};
        ret[(size_t)Opcode::Addssrm] = {"addss"};
        ret[(size_t)Opcode::Addssmr] = {"addss"};
        ret[(size_t)Opcode::Subssrr] = {"subss"};
        ret[(size_t)Opcode::Subssrm] = {"subss"};
        ret[(size_t)Opcode::Subssmr] = {"subss"};
        ret[(size_t)Opcode::Mulssrr] = {"mulss"};

        ret[(size_t)Opcode::Addsdrr] = {"addsd"};
        ret[(size_t)Opcode::Addsdrm] = {"addsd"};
        ret[(size_t)Opcode::Addsdmr] = {"addsd"};
        ret[(size_t)Opcode::Subsdrr] = {"subsd"};
        ret[(size_t)Opcode::Subsdrm] = {"subsd"};
        ret[(size_t)Opcode::Subsdmr] = {"subsd"};
        ret[(size_t)Opcode::Mulsdrr] = {"mulsd"};

        ret[(size_t)Opcode::Lea64rm] = {"lea"};
        ret[(size_t)Opcode::Lea32rm] = {"lea"};
        ret[(size_t)Opcode::Lea16rm] = {"lea"};

        ret[(size_t)Opcode::Call] = {"call"};
        ret[(size_t)Opcode::Call64r] = {"call"};
        
        ret[(size_t)Opcode::Rep_Movsb] = {"rep movsb"};

        ret[(size_t)Opcode::Movzx64r16r] = {"movzx"};
        ret[(size_t)Opcode::Movzx64r16m] = {"movzx"};
        ret[(size_t)Opcode::Movzx32r16r] = {"movzx"};
        ret[(size_t)Opcode::Movzx32r16m] = {"movzx"};
        ret[(size_t)Opcode::Movzx64r8r] = {"movzx"};
        ret[(size_t)Opcode::Movzx64r8m] = {"movzx"};
        ret[(size_t)Opcode::Movzx32r8r] = {"movzx"};
        ret[(size_t)Opcode::Movzx32r8m] = {"movzx"};
        ret[(size_t)Opcode::Movzx16r8r] = {"movzx"};
        ret[(size_t)Opcode::Movzx16r8m] = {"movzx"};
        ret[(size_t)Opcode::Movsx64r32r] = {"movsxd"};
        ret[(size_t)Opcode::Movsx64r32m] = {"movsxd"};
        ret[(size_t)Opcode::Movsx32r32r] = {"movsxd"};
        ret[(size_t)Opcode::Movsx32r32m] = {"movsxd"};
        ret[(size_t)Opcode::Movsx64r16r] = {"movsx"};
        ret[(size_t)Opcode::Movsx64r16m] = {"movsx"};
        ret[(size_t)Opcode::Movsx32r16r] = {"movsx"};
        ret[(size_t)Opcode::Movsx32r16m] = {"movsx"};
        ret[(size_t)Opcode::Movsx64r8r] = {"movsx"};
        ret[(size_t)Opcode::Movsx64r8m] = {"movsx"};
        ret[(size_t)Opcode::Movsx32r8r] = {"movsx"};
        ret[(size_t)Opcode::Movsx32r8m] = {"movsx"};
        ret[(size_t)Opcode::Movsx16r8r] = {"movsx"};
        ret[(size_t)Opcode::Movsx16r8m] = {"movsx"};
        ret[(size_t)Opcode::Cvtss2sdrr] = {"cvtss2sd"};
        ret[(size_t)Opcode::Cvtss2sdrm] = {"cvtss2sd"};
        ret[(size_t)Opcode::Cvtsd2ssrr] = {"cvtsd2ss"};
        ret[(size_t)Opcode::Cvtsd2ssrm] = {"cvtsd2ss"};
        ret[(size_t)Opcode::Cvtsi2ss64rr] = {"cvtsi2ss"};
        ret[(size_t)Opcode::Cvtsi2ss64rm] = {"cvtsi2ss"};
        ret[(size_t)Opcode::Cvtsi2ss32rr] = {"cvtsi2ss"};
        ret[(size_t)Opcode::Cvtsi2ss32rm] = {"cvtsi2ss"};
        ret[(size_t)Opcode::Cvtsi2sd64rr] = {"cvtsi2sd"};
        ret[(size_t)Opcode::Cvtsi2sd64rm] = {"cvtsi2sd"};
        ret[(size_t)Opcode::Cvtsi2sd32rr] = {"cvtsi2sd"};
        ret[(size_t)Opcode::Cvtsi2sd32rm] = {"cvtsi2sd"};
        ret[(size_t)Opcode::Cvtsd2si64rr] = {"cvtsd2si"};
        ret[(size_t)Opcode::Cvtsd2si64rm] = {"cvtsd2si"};
        ret[(size_t)Opcode::Cvtsd2si32rr] = {"cvtsd2si"};
        ret[(size_t)Opcode::Cvtsd2si32rm] = {"cvtsd2si"};
        ret[(size_t)Opcode::Cvtss2si64rr] = {"cvtss2si"};
        ret[(size_t)Opcode::Cvtss2si64rm] = {"cvtss2si"};
        ret[(size_t)Opcode::Cvtss2si32rr] = {"cvtss2si"};
        ret[(size_t)Opcode::Cvtss2si32rm] = {"cvtss2si"};

        ret[(size_t)Opcode::Shl8ri] = {"shl"};
        ret[(size_t)Opcode::Shl8rCL] = {"shl"};
        ret[(size_t)Opcode::Shl16ri] = {"shl"};
        ret[(size_t)Opcode::Shl16rCL] = {"shl"};
        ret[(size_t)Opcode::Shl32ri] = {"shl"};
        ret[(size_t)Opcode::Shl32rCL] = {"shl"};
        ret[(size_t)Opcode::Shl64ri] = {"shl"};
        ret[(size_t)Opcode::Shl64rCL] = {"shl"};

        ret[(size_t)Opcode::Shr8ri] = {"shr"};
        ret[(size_t)Opcode::Shr8rCL] = {"shr"};
        ret[(size_t)Opcode::Shr16ri] = {"shr"};
        ret[(size_t)Opcode::Shr16rCL] = {"shr"};
        ret[(size_t)Opcode::Shr32ri] = {"shr"};
        ret[(size_t)Opcode::Shr32rCL] = {"shr"};
        ret[(size_t)Opcode::Shr64ri] = {"shr"};
        ret[(size_t)Opcode::Shr64rCL] = {"shr"};

        ret[(size_t)Opcode::Sar8ri] = {"sar"};
        ret[(size_t)Opcode::Sar8rCL] = {"sar"};
        ret[(size_t)Opcode::Sar16ri] = {"sar"};
        ret[(size_t)Opcode::Sar16rCL] = {"sar"};
        ret[(size_t)Opcode::Sar32ri] = {"sar"};
        ret[(size_t)Opcode::Sar32rCL] = {"sar"};
        ret[(size_t)Opcode::Sar64ri] = {"sar"};
        ret[(size_t)Opcode::Sar64rCL] = {"sar"};

        ret[(size_t)Opcode::And8ri] = {"and"};
        ret[(size_t)Opcode::And8rr] = {"and"};
        ret[(size_t)Opcode::And16ri] = {"and"};
        ret[(size_t)Opcode::And16rr] = {"and"};
        ret[(size_t)Opcode::And32ri] = {"and"};
        ret[(size_t)Opcode::And32rr] = {"and"};
        ret[(size_t)Opcode::And64rr] = {"and"};

        ret[(size_t)Opcode::Or8ri] = {"or"};
        ret[(size_t)Opcode::Or8rr] = {"or"};
        ret[(size_t)Opcode::Or16ri] = {"or"};
        ret[(size_t)Opcode::Or16rr] = {"or"};
        ret[(size_t)Opcode::Or32ri] = {"or"};
        ret[(size_t)Opcode::Or32rr] = {"or"};
        ret[(size_t)Opcode::Or64rr] = {"or"};

        ret[(size_t)Opcode::Xor8ri] = {"xor"};
        ret[(size_t)Opcode::Xor8rr] = {"xor"};
        ret[(size_t)Opcode::Xor16ri] = {"xor"};
        ret[(size_t)Opcode::Xor16rr] = {"xor"};
        ret[(size_t)Opcode::Xor32ri] = {"xor"};
        ret[(size_t)Opcode::Xor32rr] = {"xor"};
        ret[(size_t)Opcode::Xor64rr] = {"xor"};

        ret[(size_t)Opcode::Not64r] = {"not"};
        ret[(size_t)Opcode::Not32r] = {"not"};
        ret[(size_t)Opcode::Not16r] = {"not"};
        ret[(size_t)Opcode::Not8r] = {"not"};

        ret[(size_t)Opcode::Cmpssrr] = {"cmpss"};
        ret[(size_t)Opcode::Cmpsdrr] = {"cmpsd"};

        ret[(size_t)Opcode::Ucomissrr] = {"ucomiss"};
        ret[(size_t)Opcode::Ucomisdrr] = {"ucomisd"};

        ret[(size_t)Opcode::Sete] = {"sete"};
        ret[(size_t)Opcode::Setne] = {"setne"};
        ret[(size_t)Opcode::Setl] = {"setl"};
        ret[(size_t)Opcode::Setle] = {"setle"};
        ret[(size_t)Opcode::Setg] = {"setg"};
        ret[(size_t)Opcode::Setge] = {"setge"};
        ret[(size_t)Opcode::Setb] = {"setb"};
        ret[(size_t)Opcode::Setbe] = {"setbe"};
        ret[(size_t)Opcode::Seta] = {"seta"};
        ret[(size_t)Opcode::Setae] = {"setae"};

        ret[(size_t)Opcode::Cqo] = {"cqo"};
        ret[(size_t)Opcode::Cdq] = {"cdq"};
        ret[(size_t)Opcode::Cwd] = {"cwd"};
        ret[(size_t)Opcode::Cbw] = {"cbw"};

        ret[(size_t)Opcode::IDiv64] = {"idiv"};
        ret[(size_t)Opcode::IDiv32] = {"idiv"};
        ret[(size_t)Opcode::IDiv16] = {"idiv"};
        ret[(size_t)Opcode::IDiv8] = {"idiv"};
        ret[(size_t)Opcode::Div64] = {"div"};
        ret[(size_t)Opcode::Div16] = {"div"};
        ret[(size_t)Opcode::Div8] = {"div"};
        ret[(size_t)Opcode::Div32] = {"div"};
        ret[(size_t)Opcode::Divssrr] = {"divss"};
        ret[(size_t)Opcode::Divsdrr] = {"divsd"};

        ret[(size_t)Opcode::Mul64] = {"mul"};
        ret[(size_t)Opcode::Mul32] = {"mul"};
        ret[(size_t)Opcode::Mul16] = {"mul"};
        ret[(size_t)Opcode::Mul8] = {"mul"};

        ret[(size_t)Opcode::IMul64rr] = {"imul"};
        ret[(size_t)Opcode::IMul32rr] = {"imul"};
        ret[(size_t)Opcode::IMul16rr] = {"imul"};

        ret[(size_t)Opcode::IMul64rri] = {"imul"};
        ret[(size_t)Opcode::IMul32rri] = {"imul"};
        ret[(size_t)Opcode::IMul16rri] = {"imul"};

        ret[(size_t)Opcode::Test64rr] = {"test"};
        ret[(size_t)Opcode::Test32rr] = {"test"};
        ret[(size_t)Opcode::Test16rr] = {"test"};
        ret[(size_t)Opcode::Test8rr] = {"test"};

        return ret;
    }();

    m_patterns = std::move(
        PatternBuilder()
        .forOpcode(Node::NodeKind::FunctionArgument)
            .match(matchFunctionArguments).emit(emitFunctionArguments).withName("FunctionArguments")
        .forOpcode(Node::Node::NodeKind::Register)
            .match(matchRegister).emit(emitRegister).withName("Register")
        .forOpcode(Node::NodeKind::FrameIndex)
            .match(matchFrameIndex).emit(emitFrameIndex).withName("FrameIndex")
        .forOpcode(Node::NodeKind::Root)
            .match(matchRoot).emit(emitRoot).withName("Root")
        .forOpcode(Node::NodeKind::ConstantInt)
            .match(matchConstantInt).emit(emitConstantInt).withName("ConstantInt")
        .forOpcode(Node::NodeKind::MultiValue)
            .match(matchMultiValue).emit(emitMultiValue).withName("MultiValue")
        .forOpcode(Node::NodeKind::Ret)
            .match(matchReturn).emit(emitReturn).withName("Return")
            .match(matchReturnOp).emit(emitReturnLowering).withName("ReturnLowering")
        .forOpcode(Node::NodeKind::Store)
            .match(matchStoreInFrame).emit(emitStoreInFrame).withName("StoreInFrame")
            .match(matchStoreInPtrRegister).emit(emitStoreInPtrRegister).withName("StoreInPtrRegister")
        .forOpcode(Node::NodeKind::Load)
            .match(matchLoadFromFrame).emit(emitLoadFromFrame).withName("LoadFromFrame")
            .match(matchLoadFromPtrRegister).emit(emitLoadFromPtrRegister).withName("LoadFromPtrRegister")
        .forOpcode(Node::NodeKind::Add)
            .match(matchAddImmediates).emit(emitAddImmediates).withName("AddImmediates")
            .match(matchAddRegisters).emit(emitAddRegisters).withName("AddRegisters")
            .match(matchAddRegisterImmediate).emit(emitAddRegisterImmediate).withName("AddRegisterImmediate")
        .forOpcode(Node::NodeKind::Sub)
            .match(matchSubImmediates).emit(emitSubImmediates).withName("SubImmediates")
            .match(matchSubRegisters).emit(emitSubRegisters).withName("SubRegisters")
            .match(matchSubRegisterImmediate).emit(emitSubRegisterImmediate).withName("SubRegisterImmediate")
        .forOpcode(Node::NodeKind::IDiv)
            .match(matchIDiv).emit(emitIDiv).withName("IDiv")
        .forOpcode(Node::NodeKind::UDiv)
            .match(matchUDiv).emit(emitUDiv).withName("UDiv")
        .forOpcode(Node::NodeKind::FDiv)
            .match(matchFDiv).emit(emitFDiv).withName("FDiv")
        .forOpcode(Node::NodeKind::IRem)
            .match(matchIRem).emit(emitIRem).withName("IRem")
        .forOpcode(Node::NodeKind::URem)
            .match(matchURem).emit(emitURem).withName("URem")
        .forOpcode(Node::NodeKind::Phi)
            .match(matchPhi).emit(emitPhi).withName("Phi")
        .forOpcode(Node::NodeKind::Jump)
            .match(matchJump).emit(emitJump).withName("Jump")
            .match(matchCondJumpComparisonRI).emit(emitCondJumpComparisonRI).withCoveredOperands({2}).withName("CondJumpComparisonRI")
            .match(matchCondJumpComparisonII).emit(emitCondJumpComparisonII).withCoveredOperands({2}).withName("CondJumpComparisonII")
            .match(matchCondJumpComparisonRR).emit(emitCondJumpComparisonRR).withCoveredOperands({2}).withName("CondJumpComparisonRR")
            .match(matchFCondJumpComparisonRF).emit(emitFCondJumpComparisonRR).withCoveredOperands({2}).withName("FCondJumpComparisonRF")
            .match(matchFCondJumpComparisonRR).emit(emitFCondJumpComparisonRR).withCoveredOperands({2}).withName("FCondJumpComparisonRR")
            .match(matchCondJumpRegister).emit(emitCondJumpRegister).withName("CondJumpRegister")
        .forOpcode(Node::NodeKind::LoadConstant)
            .match(matchConstantFloat).emit(emitConstantFloat).withName("ConstantFloat")
        .forOpcode(Node::NodeKind::LoadGlobal)
            .match(matchGlobalValue).emit(emitGlobalValue).withName("GlobalValue")
        .forOpcode(Node::NodeKind::GEP)
            .match(matchGEP).emit(emitGEP).withName("GEP")
        .forOpcode(Node::NodeKind::Call)
            .match(matchIntrinsicCall).emit(emitIntrinsicCall).withName("IntrinsicCall")
            .match(matchCall).emit(emitCallLowering).withName("Call")
        .forOpcode(Node::NodeKind::Zext)
            .match(matchZextTo64).emit(emitZextTo64).withName("ZextTo64")
            .match(matchZextTo32).emit(emitZextTo32).withName("ZextTo32")
            .match(matchZextTo16).emit(emitZextTo16).withName("ZextTo16")
        .forOpcode(Node::NodeKind::Sext)
            .match(matchSextTo64).emit(emitSextTo64).withName("SextTo64")
            .match(matchSextTo32).emit(emitSextTo32).withName("SextTo32")
            .match(matchSextTo16).emit(emitSextTo16).withName("SextTo16")
        .forOpcode(Node::NodeKind::Trunc)
            .match(matchTrunc).emit(emitTrunc).withName("Trunc")
        .forOpcode(Node::NodeKind::Fptrunc)
            .match(matchFptrunc).emit(emitFptrunc).withName("Fptrunc")
        .forOpcode(Node::NodeKind::Fpext)
            .match(matchFpext).emit(emitFpext).withName("Fpext")
        .forOpcode(Node::NodeKind::Fptosi)
            .match(matchFptosi).emit(emitFptosi).withName("Fptosi")
        .forOpcode(Node::NodeKind::Fptoui)
            .match(matchFptoui).emit(emitFptoui).withName("Fptoui")
        .forOpcode(Node::NodeKind::Sitofp)
            .match(matchSitofp).emit(emitSitofp).withName("Sitofp")
        .forOpcode(Node::NodeKind::Uitofp)
            .match(matchUitofp).emit(emitUitofp).withName("Uitofp")
        .forOpcode(Node::NodeKind::ShiftLeft)
            .match(matchShiftLeftImmediate).emit(emitShiftLeftImmediate).withName("ShiftLeftImmediate")
            .match(matchShiftLeftRegister).emit(emitShiftLeftRegister).withName("ShiftLeftRegister")
        .forOpcode(Node::NodeKind::LShiftRight)
            .match(matchLShiftRightImmediate).emit(emitLShiftRightImmediate).withName("LShiftRightImmediate")
            .match(matchLShiftRightRegister).emit(emitLShiftRightRegister).withName("LShiftRightRegister")
        .forOpcode(Node::NodeKind::AShiftRight)
            .match(matchAShiftRightImmediate).emit(emitAShiftRightImmediate).withName("AShiftRightImmediate")
            .match(matchAShiftRightRegister).emit(emitAShiftRightRegister).withName("AShiftRightRegister")
        .forOpcode(Node::NodeKind::And)
            .match(matchAndImmediate).emit(emitAndImmediate).withName("AndImmediate")
            .match(matchAndRegister).emit(emitAndRegister).withName("AndRegister")
        .forOpcode(Node::NodeKind::Or)
            .match(matchOrImmediate).emit(emitOrImmediate).withName("OrImmediate")
            .match(matchOrRegister).emit(emitOrRegister).withName("OrRegister")
        .forOpcode(Node::NodeKind::Xor)
            .match(matchXorImmediate).emit(emitXorImmediate).withName("XorImmediate")
            .match(matchXorRegister).emit(emitXorRegister).withName("XorRegister")
        .forOpcodes({Node::NodeKind::ICmpEq, Node::NodeKind::ICmpNe, Node::NodeKind::ICmpGt, Node::NodeKind::ICmpGe, Node::NodeKind::ICmpLt, Node::NodeKind::ICmpLe,
                    Node::NodeKind::UCmpGt, Node::NodeKind::UCmpGe, Node::NodeKind::UCmpLt, Node::NodeKind::UCmpLe,
                    Node::NodeKind::FCmpEq, Node::NodeKind::FCmpNe, Node::NodeKind::FCmpGt, Node::NodeKind::FCmpGe, Node::NodeKind::FCmpLt, Node::NodeKind::FCmpLe})
            .match(matchCmpRegisterImmediate).emit(emitCmpRegisterImmediate).withName("CmpRegisterImmediate")
            .match(matchCmpRegisterRegister).emit(emitCmpRegisterRegister).withName("CmpRegisterRegister")
            .match(matchFCmpRegisterFloat).emit(emitFCmpRegisterRegister).withName("CmpFRegisterImmediate")
            .match(matchFCmpRegisterRegister).emit(emitFCmpRegisterRegister).withName("CmpFRegisterRegister")
        .forOpcode(Node::NodeKind::IMul)
            .match(matchIMulRegisterImmediate).emit(emitIMulRegisterImmediate).withName("IMulRegisterImmediate")
            .match(matchIMulRegisterRegister).emit(emitIMulRegisterRegister).withName("IMulRegisterRegister")
        .forOpcode(Node::NodeKind::UMul)
            .match(matchUMul).emit(emitUMul).withName("UMul")
        .forOpcode(Node::NodeKind::FMul)
            .match(matchFMul).emit(emitFMul).withName("FMul")
        .forOpcode(Node::NodeKind::Switch)
            .match(matchSwitch).emit(emitSwitchLowering).withName("Switch")
        .forOpcode(Node::NodeKind::GenericCast)
            .match(matchGenericCast).emit(emitGenericCast).withName("GenericCast")
        .build()
    );
}


size_t x64InstructionInfo::registerToStackSlot(MIR::Block* block, size_t pos, MIR::Register* rr, MIR::StackSlot slot) {
    uint32_t rclassid = m_registerInfo->getRegisterIdClass(rr->getId(), block->getParentFunction()->getRegisterInfo());
    Opcode op = (Opcode)selectOpcode(slot.m_size, rclassid == RegisterClass::FPR, {OPCODE(Mov8mr), OPCODE(Mov16mr), OPCODE(Mov32mr), OPCODE(Mov64mr)}, {OPCODE(Movssmr), OPCODE(Movsdmr)});
    block->addInstructionAt(operandToMemory((uint32_t)op, rr, m_registerInfo->getRegister(RBP), -(int32_t)slot.m_offset), pos);
    return 1;
}

size_t x64InstructionInfo::stackSlotToRegister(MIR::Block* block, size_t pos, MIR::Register* rr, MIR::StackSlot slot) {
    uint32_t rclassid = m_registerInfo->getRegisterIdClass(rr->getId(), block->getParentFunction()->getRegisterInfo());
    Opcode op = (Opcode)selectOpcode(slot.m_size, rclassid == RegisterClass::FPR, {OPCODE(Mov8rm), OPCODE(Mov16rm), OPCODE(Mov32rm), OPCODE(Mov64rm)}, {OPCODE(Movssrm), OPCODE(Movsdrm)});
    block->addInstructionAt(memoryToOperand((uint32_t)op, rr, m_registerInfo->getRegister(RBP), -(int32_t)slot.m_offset), pos);
    return 1;
}

size_t x64InstructionInfo::immediateToStackSlot(MIR::Block* block, size_t pos, MIR::ImmediateInt* immediate, MIR::StackSlot slot) {
    block->addInstructionAt(operandToMemory(OPCODE(Movm64i32), immediate, m_registerInfo->getRegister(RBP), -(int32_t)slot.m_offset), pos);
    return 1;
}

size_t x64InstructionInfo::move(MIR::Block* block, size_t pos, MIR::Operand* source, MIR::Operand* destination, size_t size, bool flt) {
    assert(!(source->getKind() == MIR::Operand::Kind::FrameIndex && destination->getKind() == MIR::Operand::Kind::FrameIndex) &&
        "Cannot move memory to memory");
    Opcode op;
    if(destination->isRegister()) {
        if(source->isRegister())
            op = (Opcode)selectOpcode(size, flt, {OPCODE(Mov8rr), OPCODE(Mov16rr), OPCODE(Mov32rr), OPCODE(Mov64rr)}, {OPCODE(Movssrr), OPCODE(Movsdrr)});
        else if(source->isImmediateInt()) {
            op = (Opcode)selectOpcode(size, flt, {OPCODE(Mov8ri), OPCODE(Mov16ri), OPCODE(Mov32ri), 0}, {});
            if((uint32_t)op == 0) {
                op = immSizeFromValue(cast<MIR::ImmediateInt>(source)->getValue()) > 4 ? Opcode::Movr64i64 : Opcode::Movr64i32;
            }
        }
        else if(source->isFrameIndex()) {
            MIR::Register* rr = cast<MIR::Register>(destination);
            MIR::FrameIndex* fr = cast<MIR::FrameIndex>(source);
            return stackSlotToRegister(block, pos, rr, block->getParentFunction()->getStackFrame().getStackSlot(fr->getIndex()));
        }
        else {
            assert(false && "Unreachable");
        }
    }
    else if(destination->isFrameIndex()) {
        if(source->isRegister()) {
            MIR::FrameIndex* fr = cast<MIR::FrameIndex>(destination);
            MIR::Register* rr = cast<MIR::Register>(source);
            return registerToStackSlot(block, pos, rr, block->getParentFunction()->getStackFrame().getStackSlot(fr->getIndex()));
        }
        else if(source->isImmediateInt()) {
            MIR::FrameIndex* fr = cast<MIR::FrameIndex>(destination);
            MIR::ImmediateInt* imm = cast<MIR::ImmediateInt>(source);
            return immediateToStackSlot(block, pos, imm, block->getParentFunction()->getStackFrame().getStackSlot(fr->getIndex()));
        }
        else {
            assert(false && "Unreachable");
        }
    }
    else {
        assert(false && "Unreachable");
    }
    block->addInstructionAt(instr((uint32_t)op, destination, source), pos);
    return 1;
}


size_t x64InstructionInfo::stackSlotAddress(MIR::Block* block, size_t pos, MIR::StackSlot slot, MIR::Register* destination) {
    block->addInstructionAt(memoryToOperand(OPCODE(Lea64rm), destination, m_registerInfo->getRegister(RBP), -(int32_t)slot.m_offset), pos);
    return 1;
}

std::unique_ptr<MIR::Instruction> x64InstructionInfo::operandToMemory(uint32_t op, MIR::Operand* operand, MIR::Register* base, int64_t offset, MIR::Register* index, size_t scale, MIR::Symbol* symbol) {
    return std::make_unique<MIR::Instruction>(op, base, m_ctx->getImmediateInt(scale, MIR::ImmediateInt::imm8), index, m_ctx->getImmediateInt(offset, MIR::ImmediateInt::imm32), symbol, operand);
}

std::unique_ptr<MIR::Instruction> x64InstructionInfo::memoryToOperand(uint32_t op, MIR::Operand* operand, MIR::Register* base, int64_t offset, MIR::Register* index, size_t scale, MIR::Symbol* symbol) {
    return std::make_unique<MIR::Instruction>(op, operand, base, m_ctx->getImmediateInt(scale, MIR::ImmediateInt::imm8), index, m_ctx->getImmediateInt(offset, MIR::ImmediateInt::imm32), symbol);
}

}