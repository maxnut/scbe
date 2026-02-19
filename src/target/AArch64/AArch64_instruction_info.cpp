#include "target/AArch64/AArch64_instruction_info.hpp"
#include "MIR/function.hpp"
#include "target/AArch64/AArch64_patterns.hpp"
#include "target/AArch64/AArch64_register_info.hpp"
#include "target/instruction_info.hpp"
#include "target/register_info.hpp"
#include "target/instruction_utils.hpp"
#include <cstddef>
#include <cstdint>

namespace scbe::Target::AArch64 {

using namespace ISel;

#define MEMORY_RESTRICTION Restriction::reg(), Restriction({(uint32_t)MIR::Operand::Kind::Register, (uint32_t)MIR::Operand::Kind::ImmediateInt}), Restriction::imm(), Restriction::sym()

AArch64InstructionInfo::AArch64InstructionInfo(RegisterInfo* registerInfo, Ref<Context> ctx) : InstructionInfo(registerInfo, ctx) {
    m_instructionDescriptors = []{
        std::vector<InstructionDescriptor> ret((size_t)Opcode::Count);
        // name, numDefs, numOps, size, isStore, isLoad, restrictions
        ret[(size_t)Opcode::Orr64rr] = InstructionDescriptor("Orr64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Orr32rr] = InstructionDescriptor("Orr32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Orr64ri] = InstructionDescriptor("Orr64ri", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Orr32ri] = InstructionDescriptor("Orr32ri", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});

        ret[(size_t)Opcode::Movk64ri] = InstructionDescriptor("Movk64ri", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::imm()});
        ret[(size_t)Opcode::Movk32ri] = InstructionDescriptor("Movk64ri", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::imm()});
        ret[(size_t)Opcode::Movz64ri] = InstructionDescriptor("Movz64ri", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::imm()});
        ret[(size_t)Opcode::Movz32ri] = InstructionDescriptor("Movz64ri", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::imm()});
        ret[(size_t)Opcode::Movn64ri] = InstructionDescriptor("Movn64ri", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::imm()});
        ret[(size_t)Opcode::Movn32ri] = InstructionDescriptor("Movn64ri", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::imm()});

        ret[(size_t)Opcode::Add64rr] = InstructionDescriptor("Add64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Add32rr] = InstructionDescriptor("Add32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Sub64rr] = InstructionDescriptor("Sub64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Sub32rr] = InstructionDescriptor("Sub32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Add64ri] = InstructionDescriptor("Add64ri", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Add32ri] = InstructionDescriptor("Add32ri", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Sub64ri] = InstructionDescriptor("Sub64ri", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Sub32ri] = InstructionDescriptor("Sub32ri", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Mul64rr] = InstructionDescriptor("Mul64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Mul32rr] = InstructionDescriptor("Mul32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});

        ret[(size_t)Opcode::Load64rm] = InstructionDescriptor("Load64rm", 1, 5, 8, false, true, {Restriction::reg(true), MEMORY_RESTRICTION});
        ret[(size_t)Opcode::Load32rm] = InstructionDescriptor("Load32rm", 1, 5, 4, false, true, {Restriction::reg(true), MEMORY_RESTRICTION});

        ret[(size_t)Opcode::Store64rm] = InstructionDescriptor("Store64rm", 0, 5, 8, true, false, {Restriction::reg(), MEMORY_RESTRICTION});
        ret[(size_t)Opcode::Store32rm] = InstructionDescriptor("Store32rm", 0, 5, 4, true, false, {Restriction::reg(), MEMORY_RESTRICTION});

        ret[(size_t)Opcode::LoadP64rm] = InstructionDescriptor("LoadP64rm", 2, 6, 8, false, true, {Restriction::reg(true), Restriction::reg(true), MEMORY_RESTRICTION});
        ret[(size_t)Opcode::LoadP32rm] = InstructionDescriptor("LoadP32rm", 2, 6, 4, false, true, {Restriction::reg(true), Restriction::reg(true), MEMORY_RESTRICTION});

        ret[(size_t)Opcode::StoreP64rm] = InstructionDescriptor("StoreP64rm", 0, 6, 8, true, false, {Restriction::reg(), Restriction::reg(true), MEMORY_RESTRICTION});
        ret[(size_t)Opcode::StoreP32rm] = InstructionDescriptor("StoreP32rm", 0, 6, 4, true, false, {Restriction::reg(), Restriction::reg(true), MEMORY_RESTRICTION});

        ret[(size_t)Opcode::Ret] = InstructionDescriptor("Ret", 0, 0, 0, false, false, {}, {}, true);

        ret[(size_t)Opcode::B] = InstructionDescriptor("B", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Beq] = InstructionDescriptor("Beq", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bge] = InstructionDescriptor("Bge", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bgt] = InstructionDescriptor("Bgt", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Ble] = InstructionDescriptor("Ble", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Blt] = InstructionDescriptor("Blt", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bne] = InstructionDescriptor("Bne", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Br] = InstructionDescriptor("Br", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bhi] = InstructionDescriptor("Bhi", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bls] = InstructionDescriptor("Bls", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bcs] = InstructionDescriptor("Bcs", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);
        ret[(size_t)Opcode::Bcc] = InstructionDescriptor("Bcc", 0, 1, 8, false, false, {Restriction::sym()}, {}, false, true);

        ret[(size_t)Opcode::Subs64rr] = InstructionDescriptor("Subs64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Subs32rr] = InstructionDescriptor("Subs32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Subs64ri] = InstructionDescriptor("Subs64ri", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Subs32ri] = InstructionDescriptor("Subs32ri", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});

        ret[(size_t)Opcode::Adrp] = InstructionDescriptor("Adrp", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::sym()});

        ret[(size_t)Opcode::Fmov64rr] = InstructionDescriptor("Fmov64rr", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Fmov32rr] = InstructionDescriptor("Fmov32rr", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()});

        ret[(size_t)Opcode::Fadd64rr] = InstructionDescriptor("Fadd64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fadd32rr] = InstructionDescriptor("Fadd32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fsub64rr] = InstructionDescriptor("Fsub64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fsub32rr] = InstructionDescriptor("Fsub32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fmul64rr] = InstructionDescriptor("Fmul64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fmul32rr] = InstructionDescriptor("Fmul32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fdiv64rr] = InstructionDescriptor("Fdiv64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fdiv32rr] = InstructionDescriptor("Fdiv32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});

        ret[(size_t)Opcode::Call] = {"Call", 0, 1, 8, false, false, {Restriction({(uint32_t)MIR::Operand::Kind::GlobalAddress, (uint32_t)MIR::Operand::Kind::ExternalSymbol}, false)}, {
            X0, X1, X2, X3, X4, X5, X6, X7,
            X9, X10, X11, X12, X13, X14, X15
        }, false, false, true};
        ret[(size_t)Opcode::Call64r] = {"Call64r", 0, 1, 8, false, false, {Restriction::reg()}, {
            X0, X1, X2, X3, X4, X5, X6, X7,
            X9, X10, X11, X12, X13, X14, X15
        }, false, false, true};

        ret[(size_t)Opcode::And64rr] = InstructionDescriptor("And64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::And64ri] = InstructionDescriptor("And64ri", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::And32rr] = InstructionDescriptor("And32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::And32ri] = InstructionDescriptor("And32ri", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Eor64rr] = InstructionDescriptor("Eor64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Eor64ri] = InstructionDescriptor("Eor64ri", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Eor32rr] = InstructionDescriptor("Eor32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Eor32ri] = InstructionDescriptor("Eor32ri", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Sbfm64] = InstructionDescriptor("Sbfm64", 1, 4, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm(), Restriction::imm()});
        ret[(size_t)Opcode::Sbfm32] = InstructionDescriptor("Sbfm32", 1, 4, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm(), Restriction::imm()});
        ret[(size_t)Opcode::Ubfm64] = InstructionDescriptor("Ubfm64", 1, 4, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm(), Restriction::imm()});
        ret[(size_t)Opcode::Ubfm32] = InstructionDescriptor("Ubfm32", 1, 4, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::imm(), Restriction::imm()});
        ret[(size_t)Opcode::Fcvt] = InstructionDescriptor("Fcvt", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Fcvtzs64] = InstructionDescriptor("Fcvtzs64", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Fcvtzu64] = InstructionDescriptor("Fcvtzu64", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Scvtf64] = InstructionDescriptor("Scvtf64", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Ucvtf64] = InstructionDescriptor("Ucvtf64", 1, 2, 8, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Fcvtzs32] = InstructionDescriptor("Fcvtzs32", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Fcvtzu32] = InstructionDescriptor("Fcvtzu32", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Scvtf32] = InstructionDescriptor("Scvtf32", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Ucvtf32] = InstructionDescriptor("Ucvtf32", 1, 2, 4, false, false, {Restriction::reg(true), Restriction::reg()});
        ret[(size_t)Opcode::Lslv64] = InstructionDescriptor("Lslv64", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Lslv32] = InstructionDescriptor("Lslv32", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Lsrv64] = InstructionDescriptor("Lsrv64", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Lsrv32] = InstructionDescriptor("Lsrv32", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Asrv64] = InstructionDescriptor("Asrv64", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Asrv32] = InstructionDescriptor("Asrv32", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});

        ret[(size_t)Opcode::Fcmp64rr] = InstructionDescriptor("Fcmp64rr", 1, 2, 8, false, false, {Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Fcmp32rr] = InstructionDescriptor("Fcmp32rr", 1, 2, 4, false, false, {Restriction::reg(), Restriction::reg()});

        ret[(size_t)Opcode::Csinc64] = InstructionDescriptor("Csinc64", 1, 4, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg(), Restriction::imm()});
        ret[(size_t)Opcode::Csinc32] = InstructionDescriptor("Csinc32", 1, 4, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg(), Restriction::imm()});

        ret[(size_t)Opcode::Sdiv64rr] = InstructionDescriptor("Sdiv64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Sdiv32rr] = InstructionDescriptor("Sdiv32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});

        ret[(size_t)Opcode::Udiv64rr] = InstructionDescriptor("Udiv64rr", 1, 3, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Udiv32rr] = InstructionDescriptor("Udiv32rr", 1, 3, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg()});

        ret[(size_t)Opcode::Msub64] = InstructionDescriptor("Msub64", 1, 4, 8, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg(), Restriction::reg()});
        ret[(size_t)Opcode::Msub32] = InstructionDescriptor("Msub64", 1, 4, 4, false, false, {Restriction::reg(true), Restriction::reg(), Restriction::reg(), Restriction::reg()});

        return ret;
    }();

    m_mnemonics = []{
        std::vector<Mnemonic> ret((size_t)Opcode::Count);
        ret[(size_t)Opcode::Orr64rr] = {"orr"};
        ret[(size_t)Opcode::Orr32rr] = {"orr"};
        ret[(size_t)Opcode::Orr64ri] = {"orr"};
        ret[(size_t)Opcode::Orr32ri] = {"orr"};

        ret[(size_t)Opcode::Movk64ri] = {"movk"};
        ret[(size_t)Opcode::Movk32ri] = {"movk"};
        ret[(size_t)Opcode::Movz64ri] = {"movz"};
        ret[(size_t)Opcode::Movz32ri] = {"movz"};
        ret[(size_t)Opcode::Movn64ri] = {"movn"};
        ret[(size_t)Opcode::Movn32ri] = {"movn"};

        ret[(size_t)Opcode::Add64rr] = {"add"};
        ret[(size_t)Opcode::Add32rr] = {"add"};
        ret[(size_t)Opcode::Sub64rr] = {"sub"};
        ret[(size_t)Opcode::Sub32rr] = {"sub"};
        ret[(size_t)Opcode::Add64ri] = {"add"};
        ret[(size_t)Opcode::Add32ri] = {"add"};
        ret[(size_t)Opcode::Sub64ri] = {"sub"};
        ret[(size_t)Opcode::Sub32ri] = {"sub"};
        ret[(size_t)Opcode::Mul64rr] = {"mul"};
        ret[(size_t)Opcode::Mul32rr] = {"mul"};

        ret[(size_t)Opcode::Load64rm] = {"ldr"};
        ret[(size_t)Opcode::Load32rm] = {"ldr"};

        ret[(size_t)Opcode::Store64rm] = {"str"};
        ret[(size_t)Opcode::Store32rm] = {"str"};

        ret[(size_t)Opcode::LoadP64rm] = {"ldp"};
        ret[(size_t)Opcode::LoadP32rm] = {"ldp"};

        ret[(size_t)Opcode::StoreP64rm] = {"stp"};
        ret[(size_t)Opcode::StoreP32rm] = {"stp"};

        ret[(size_t)Opcode::Ret] = {"ret"};

        ret[(size_t)Opcode::B] = {"b"};
        ret[(size_t)Opcode::Beq] = {"beq"};
        ret[(size_t)Opcode::Bge] = {"bge"};
        ret[(size_t)Opcode::Bgt] = {"bgt"}; 
        ret[(size_t)Opcode::Ble] =  {"ble"};
        ret[(size_t)Opcode::Blt] =  {"blt"};
        ret[(size_t)Opcode::Bne] = {"bne"};
        ret[(size_t)Opcode::Br] = {"br"};
        ret[(size_t)Opcode::Bhi] = {"bhi"};
        ret[(size_t)Opcode::Bls] = {"bls"};
        ret[(size_t)Opcode::Bcs] = {"bcs"};
        ret[(size_t)Opcode::Bcc] = {"bcc"};

        ret[(size_t)Opcode::Subs64rr] = {"subs"};
        ret[(size_t)Opcode::Subs32rr] = {"subs"};
        ret[(size_t)Opcode::Subs64ri] = {"subs"};
        ret[(size_t)Opcode::Subs32ri] = {"subs"};

        ret[(size_t)Opcode::Adrp] = {"adrp"};

        ret[(size_t)Opcode::Fmov64rr] = {"fmov"};
        ret[(size_t)Opcode::Fmov32rr] = {"fmov"};

        ret[(size_t)Opcode::Fadd64rr] = {"fadd"};
        ret[(size_t)Opcode::Fadd32rr] = {"fadd"};

        ret[(size_t)Opcode::Fsub64rr] = {"fsub"};
        ret[(size_t)Opcode::Fsub32rr] = {"fsub"};

        ret[(size_t)Opcode::Fmul64rr] = {"fmul"};
        ret[(size_t)Opcode::Fmul32rr] = {"fmul"};

        ret[(size_t)Opcode::Fdiv64rr] = {"fdiv"};
        ret[(size_t)Opcode::Fdiv32rr] = {"fdiv"};
        
        ret[(size_t)Opcode::Call] = {"bl"};
        ret[(size_t)Opcode::Call64r] = {"blr"};

        ret[(size_t)Opcode::And64rr] = {"and"};
        ret[(size_t)Opcode::And64ri] = {"and"};
        ret[(size_t)Opcode::And32rr] = {"and"};
        ret[(size_t)Opcode::And32ri] = {"and"};
        ret[(size_t)Opcode::Eor64rr] = {"eor"};
        ret[(size_t)Opcode::Eor64ri] = {"eor"};
        ret[(size_t)Opcode::Eor32rr] = {"eor"};
        ret[(size_t)Opcode::Eor32ri] = {"eor"};
        ret[(size_t)Opcode::Sbfm64] = {"sbfm"};
        ret[(size_t)Opcode::Sbfm32] = {"sbfm"};
        ret[(size_t)Opcode::Ubfm64] = {"ubfm"};
        ret[(size_t)Opcode::Ubfm32] = {"ubfm"};
        ret[(size_t)Opcode::Fcvt] = {"fcvt"};
        ret[(size_t)Opcode::Fcvtzs64] = {"fcvtzs"};
        ret[(size_t)Opcode::Fcvtzu64] = {"fcvtzu"};
        ret[(size_t)Opcode::Scvtf64] = {"scvtf"};
        ret[(size_t)Opcode::Ucvtf64] = {"ucvtf"};
        ret[(size_t)Opcode::Fcvtzs32] = {"fcvtzs"};
        ret[(size_t)Opcode::Fcvtzu32] = {"fcvtzu"};
        ret[(size_t)Opcode::Scvtf32] = {"scvtf"};
        ret[(size_t)Opcode::Ucvtf32] = {"ucvtf"};
        ret[(size_t)Opcode::Lslv64] = {"lslv"};
        ret[(size_t)Opcode::Lslv32] = {"lslv"};
        ret[(size_t)Opcode::Lsrv64] = {"lsrv"};
        ret[(size_t)Opcode::Lsrv32] = {"lsrv"};
        ret[(size_t)Opcode::Asrv64] = {"asrv"};
        ret[(size_t)Opcode::Asrv32] = {"asrv"};

        ret[(size_t)Opcode::Fcmp64rr] = {"fcmp"};
        ret[(size_t)Opcode::Fcmp32rr] = {"fcmp"};

        ret[(size_t)Opcode::Csinc64] = {"csinc"};
        ret[(size_t)Opcode::Csinc32] = {"csinc"};

        ret[(size_t)Opcode::Sdiv64rr] = {"sdiv"};
        ret[(size_t)Opcode::Sdiv32rr] = {"sdiv"};
        ret[(size_t)Opcode::Udiv64rr] = {"udiv"};
        ret[(size_t)Opcode::Udiv32rr] = {"udiv"};

        ret[(size_t)Opcode::Msub64] = {"msub"};
        ret[(size_t)Opcode::Msub32] = {"msub"};        

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
        .forOpcode(Node::NodeKind::ExtractValue)
            .match(matchExtractValue).emit(emitExtractValue).withName("ExtractValue")
        .forOpcode(Node::NodeKind::DynamicAllocation)
            .match(matchDynamicAllocation).emit(emitDynamicAllocation).withName("DynamicAllocation")
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
        .forOpcodes({Node::NodeKind::IMul, Node::NodeKind::UMul, Node::NodeKind::FMul})
            .match(matchMulImmediates).emit(emitMulImmediates).withName("MulImmediates")
            .match(matchMulRegisters).emit(emitMulRegisters).withName("MulRegisters")
            .match(matchMulRegisterImmediate).emit(emitMulRegisterImmediate).withName("MulRegisterImmediate")
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
            .match(matchCondJumpImmediate).emit(emitCondJumpImmediate).withName("CondJumpImmediate")
        .forOpcode(Node::NodeKind::ConstantFloat)
            .match(matchConstantFloat).emit(emitConstantFloat).withName("ConstantFloat")
        .forOpcode(Node::NodeKind::GlobalValue)
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
            .match(matchZextTo8).emit(emitZextTo8).withName("ZextTo8")
        .forOpcode(Node::NodeKind::Sext)
            .match(matchSextTo64).emit(emitSextTo64).withName("SextTo64")
            .match(matchSextTo32or16or8).emit(emitSextTo32or16or8).withName("SextTo32or16or8")
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
            .match(matchCmpImmediateImmediate).emit(emitCmpImmediateImmediate).withName("CmpImmediateImmediate")
            .match(matchFCmpRegisterFloat).emit(emitFCmpRegisterRegister).withName("CmpFRegisterImmediate")
            .match(matchFCmpRegisterRegister).emit(emitFCmpRegisterRegister).withName("CmpFRegisterRegister")
        .forOpcode(Node::NodeKind::Switch)
            .match(matchSwitch).emit(emitSwitchLowering).withName("Switch")
        .forOpcode(Node::NodeKind::GenericCast)
            .match(matchGenericCast).emit(emitGenericCast).withName("GenericCast")
        .build()
    );
}

size_t AArch64InstructionInfo::registerToStackSlot(MIR::Block* block, size_t pos, MIR::Register* rr, MIR::StackSlot slot) {
    Opcode op = slot.m_size > 4 ? Opcode::Store64rm : Opcode::Store32rm;
    registerMemoryOp(block, pos, op, rr, m_registerInfo->getRegister(X29), -(int32_t)slot.m_offset);
    return 1;
}

size_t AArch64InstructionInfo::stackSlotToRegister(MIR::Block* block, size_t pos, MIR::Register* rr, MIR::StackSlot slot) {
    Opcode op = slot.m_size > 4 ? Opcode::Load64rm : Opcode::Load32rm;
    registerMemoryOp(block, pos, op, rr, m_registerInfo->getRegister(X29), -(int32_t)slot.m_offset);
    return 1;
}

size_t AArch64InstructionInfo::immediateToStackSlot(MIR::Block* block, size_t pos, MIR::ImmediateInt* immediate, MIR::StackSlot stackSlot) {
    size_t total = 0;
    RegisterClass rclass = immediate->getSize() <= 4 ? GPR32 : GPR64;
    MIR::Register* dest = m_registerInfo->getRegister(getRegisterInfo()->getReservedRegisters(rclass).back());
    total += move(block, pos, immediate, dest, immediate->getSize(), false);
    registerMemoryOp(block, pos, immediate->getSize() > 4 ? Opcode::Store64rm : Opcode::Store32rm, dest, m_registerInfo->getRegister(X29), -(int32_t)stackSlot.m_offset);
    return total + 1;
}

size_t AArch64InstructionInfo::registerMemoryOp(MIR::Block* block, size_t pos, Opcode opcode, MIR::Register* op, MIR::Register* base, int64_t offset, Indexing indexing, MIR::Symbol* symbol) {
    block->addInstructionAt(instr((uint32_t)opcode, op, base, getImmediate(block, pos, m_ctx->getImmediateInt(offset, MIR::ImmediateInt::imm8)), m_ctx->getImmediateInt((int64_t)indexing, MIR::ImmediateInt::imm8), symbol), pos);
    return 1;
}

size_t AArch64InstructionInfo::registerMemoryOp(MIR::Block* block, size_t pos, Opcode opcode, MIR::Register* op, MIR::Register* base, MIR::Register* offset, Indexing indexing, MIR::Symbol* symbol) {
    block->addInstructionAt(instr((uint32_t)opcode, op, base, offset, m_ctx->getImmediateInt((int64_t)indexing, MIR::ImmediateInt::imm8), symbol), pos);
    return 1;
}

size_t AArch64InstructionInfo::registersMemoryOp(MIR::Block* block, size_t pos, Opcode opcode, std::initializer_list<MIR::Register*> ops, MIR::Register* base, int64_t offset, Indexing indexing, MIR::Symbol* symbol) {
    std::unique_ptr<MIR::Instruction> ins = instr((uint32_t)opcode);
    for(auto op : ops)
        ins->addOperand(op);
    ins->addOperand(base);
    ins->addOperand(getImmediate(block, pos, m_ctx->getImmediateInt(offset, MIR::ImmediateInt::imm8)));
    ins->addOperand(m_ctx->getImmediateInt((int64_t)indexing, MIR::ImmediateInt::imm8));
    ins->addOperand(symbol);
    block->addInstructionAt(std::move(ins), pos);
    return 1;
}

size_t AArch64InstructionInfo::registersMemoryOp(MIR::Block* block, size_t pos, Opcode opcode, std::initializer_list<MIR::Register*> ops, MIR::Register* base, MIR::Register* offset, Indexing indexing, MIR::Symbol* symbol) {
    std::unique_ptr<MIR::Instruction> ins = instr((uint32_t)opcode);
    for(auto op : ops)
        ins->addOperand(op);
    ins->addOperand(base);
    ins->addOperand(offset);
    ins->addOperand(m_ctx->getImmediateInt((int64_t)indexing, MIR::ImmediateInt::imm8));
    ins->addOperand(symbol);
    block->addInstructionAt(std::move(ins), pos);
    return 1;
}

size_t AArch64InstructionInfo::stackSlotAddress(MIR::Block* block, size_t pos, MIR::StackSlot slot, MIR::Register* destination) {
    MIR::ImmediateInt::Size size = immSizeFromValue(slot.m_offset);
    auto immediate = getImmediate(block, pos, m_ctx->getImmediateInt(slot.m_offset, size));
    Opcode op = size > 4 ? Opcode::Sub64rr : Opcode::Sub32rr;
    block->addInstructionAt(
        instr((uint32_t)op, destination, m_registerInfo->getRegister(X29), immediate),
        pos
    );
    return 1;
}

MIR::Operand* AArch64InstructionInfo::getImmediate(MIR::Block* block, MIR::ImmediateInt* imm) {
    size_t tmp = block->last();
    return getImmediate(block, tmp, imm);
}

MIR::Operand* AArch64InstructionInfo::getImmediate(MIR::Block* block, size_t& pos, MIR::ImmediateInt* immediate) {
    if(immediate->getSize() == MIR::ImmediateInt::imm8) return immediate;
    uint64_t value = immediate->getValue();
    size_t size = immediate->getSize();

    bool is32 = (size <= 4);
    RegisterClass rclass = is32 ? GPR32 : GPR64;
    MIR::Register* dest = m_registerInfo->getRegister(getRegisterInfo()->getReservedRegisters(rclass).back());

    bool useMovn = (~value & 0xFFFF) < (value & 0xFFFF);
    uint64_t base = useMovn ? ~value : value;

    Opcode movz = is32 ? Opcode::Movz32ri : Opcode::Movz64ri;
    Opcode movn = is32 ? Opcode::Movn32ri : Opcode::Movn64ri;
    Opcode movk = is32 ? Opcode::Movk32ri : Opcode::Movk64ri;


    for (int i = 0; i < (is32 ? 2 : 4); ++i) {
        uint16_t chunk = (base >> (16 * i)) & 0xFFFF;
        MIR::Operand* shift = m_ctx->getImmediateInt(i * 16, MIR::ImmediateInt::imm8, ShiftLeft);
        if (chunk != 0 || i == 0) {
            block->addInstructionAt(
                instr((uint32_t)(useMovn ? movn : movz), dest, m_ctx->getImmediateInt(chunk, MIR::ImmediateInt::imm16), shift),
                pos++
            );
            break;
        }
    }

    for (int i = 0; i < (is32 ? 2 : 4); ++i) {
        MIR::Operand* shift = m_ctx->getImmediateInt(i * 16, MIR::ImmediateInt::imm8, ShiftLeft);
        uint16_t chunk = (value >> (16 * i)) & 0xFFFF;
        if (chunk == ((useMovn ? ~0 : 0) & 0xFFFF)) continue;
        if (i == 0) continue; // already done in movz/movn
        block->addInstructionAt(
            instr((uint32_t)movk, dest, m_ctx->getImmediateInt(chunk, MIR::ImmediateInt::imm16), shift),
            pos++
        );
    }

    return dest;
}

size_t AArch64InstructionInfo::move(MIR::Block* block, size_t pos, MIR::Operand* source, MIR::Operand* destination, size_t size, bool flt) {
    size_t total = 0;
    if(source->isImmediateInt()) {
        size_t posOld = pos;
        source = getImmediate(block, pos, cast<MIR::ImmediateInt>(source));
        total += pos - posOld;
    }
    
    if(destination->isRegister()) {
        if(source->isRegister()) {
            MIR::Register* srcReg = cast<MIR::Register>(source);
            MIR::Register* dstReg = cast<MIR::Register>(destination);
            if(srcReg->getId() == SP || dstReg->getId() == SP) {
                block->addInstructionAt(instr((uint32_t)Opcode::Add32ri, dstReg, srcReg, m_ctx->getImmediateInt(0, MIR::ImmediateInt::imm8)), pos);
                total++;
            }
            else if(!flt) {
                uint32_t zero = size > 4 ? XZR : WZR;
                Opcode op = size > 4 ? Opcode::Orr64rr : Opcode::Orr32rr;
                block->addInstructionAt(instr((uint32_t)op, dstReg, srcReg, m_registerInfo->getRegister(zero)), pos);
                total++;
            }
            else {
                Opcode op = size > 4 ? Opcode::Fmov64rr : Opcode::Fmov32rr;
                block->addInstructionAt(instr((uint32_t)op, dstReg, srcReg), pos);
                total++;
            }
            return total;
        }
        else if(source->isFrameIndex()) {
            MIR::FrameIndex* fr = cast<MIR::FrameIndex>(source);
            MIR::Register* rr = cast<MIR::Register>(destination);
            total += stackSlotToRegister(block, pos, rr, block->getParentFunction()->getStackFrame().getStackSlot(fr->getIndex()));
            return total;
        }
        else if(source->isImmediateInt()) {
            MIR::ImmediateInt* imm = cast<MIR::ImmediateInt>(source);
            MIR::Register* rr = cast<MIR::Register>(destination);
            uint32_t rclassid = m_registerInfo->getRegisterIdClass(rr->getId(), block->getParentFunction()->getRegisterInfo());
            const ::scbe::Target::RegisterClass rclass = m_registerInfo->getRegisterClass(rclassid);
            Opcode op = rclass.getSize() > 4 ? Opcode::Movz64ri : Opcode::Movz32ri;
            block->addInstructionAt(instr((uint32_t)op, rr, imm), pos);
            total++;
            return total;
        }
    }
    else if(destination->isFrameIndex()) {
        if(source->isRegister()) {
            MIR::FrameIndex* fr = cast<MIR::FrameIndex>(destination);
            MIR::Register* rr = cast<MIR::Register>(source);
            total += registerToStackSlot(block, pos, rr, block->getParentFunction()->getStackFrame().getStackSlot(fr->getIndex()));
            return total;
        }
        else if(source->isImmediateInt()) {
            MIR::FrameIndex* fr = cast<MIR::FrameIndex>(destination);
            MIR::ImmediateInt* imm = cast<MIR::ImmediateInt>(source);
            total += immediateToStackSlot(block, pos, imm, block->getParentFunction()->getStackFrame().getStackSlot(fr->getIndex()));
            return total;
        }
    }
    throw std::runtime_error("Not implemented");
}

size_t AArch64InstructionInfo::getSymbolValue(MIR::Block* block, size_t pos, MIR::Symbol* symbol, MIR::Register* dst) {
    MIR::Register* tmp = m_registerInfo->getRegister(m_registerInfo->getReservedRegisters(GPR64).back());
    block->addInstruction(instr((uint32_t)Opcode::Adrp, tmp, symbol));
    size_t total = 1;
    MIR::Symbol* symbolLow = cast<MIR::Symbol>(block->getParentFunction()->cloneOpWithFlags(symbol, ExprModLow12));
    size_t size = m_registerInfo->getRegisterClass(
        m_registerInfo->getRegisterIdClass(dst->getId(), block->getParentFunction()->getRegisterInfo())
    ).getSize();
    total += registerMemoryOp(block, block->last(), size == 8 ? Opcode::Load64rm : Opcode::Load32rm, dst, tmp, (int64_t)0, Indexing::None, symbolLow);
    return total;
}

size_t AArch64InstructionInfo::getSymbolAddress(MIR::Block* block, size_t pos, MIR::Symbol* symbol, MIR::Register* dst) {
    block->addInstruction(instr((uint32_t)Opcode::Adrp, dst, symbol));
    MIR::Symbol* symbolLow = cast<MIR::Symbol>(block->getParentFunction()->cloneOpWithFlags(symbol, ExprModLow12));
    block->addInstruction(instr((uint32_t)Opcode::Add64rr, dst, dst, symbolLow));
    return 2;
}

}