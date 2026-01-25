#include "target/x64/encoding/x64_mapping.hpp"
#include "MIR/operand.hpp"
#include "target/x64/encoding/x64_instruction_encoding.hpp"
#include "target/x64/x64_instruction_info.hpp"
#include "target/x64/x64_register_info.hpp"

using namespace scbe::MIR;

namespace scbe::Target::x64::Mapping {

const std::array<uint8_t, RegisterId::Count> s_registerMapping = [] {
    std::array<uint8_t, RegisterId::Count> table;
    table[RegisterId::RAX] = 0b0000;
    table[RegisterId::RCX] = 0b0001;
    table[RegisterId::RDX] = 0b0010;
    table[RegisterId::RBX] = 0b0011;
    table[RegisterId::RSP] = 0b0100;
    table[RegisterId::RBP] = 0b0101;
    table[RegisterId::RSI] = 0b0110;
    table[RegisterId::RDI] = 0b0111;
    table[RegisterId::R8]  = 0b1000;
    table[RegisterId::R9]  = 0b1001;
    table[RegisterId::R10] = 0b1010;
    table[RegisterId::R11] = 0b1011;
    table[RegisterId::R12] = 0b1100;
    table[RegisterId::R13] = 0b1101;
    table[RegisterId::R14] = 0b1110;
    table[RegisterId::R15] = 0b1111;
    table[RegisterId::EAX] = 0b0000;
    table[RegisterId::ECX] = 0b0001;
    table[RegisterId::EDX] = 0b0010;
    table[RegisterId::EBX] = 0b0011;
    table[RegisterId::ESP] = 0b0100;
    table[RegisterId::EBP] = 0b0101;
    table[RegisterId::ESI] = 0b0110;
    table[RegisterId::EDI] = 0b0111;
    table[RegisterId::R8D]  = 0b1000;
    table[RegisterId::R9D]  = 0b1001;
    table[RegisterId::R10D] = 0b1010;
    table[RegisterId::R11D] = 0b1011;
    table[RegisterId::R12D] = 0b1100;
    table[RegisterId::R13D] = 0b1101;
    table[RegisterId::R14D] = 0b1110;
    table[RegisterId::R15D] = 0b1111;
    table[RegisterId::AX] = 0b0000;
    table[RegisterId::CX] = 0b0001;
    table[RegisterId::DX] = 0b0010;
    table[RegisterId::BX] = 0b0011;
    table[RegisterId::SP] = 0b0100;
    table[RegisterId::BP] = 0b0101;
    table[RegisterId::SI] = 0b0110;
    table[RegisterId::DI] = 0b0111;
    table[RegisterId::R8W]  = 0b1000;
    table[RegisterId::R9W]  = 0b1001;
    table[RegisterId::R10W] = 0b1010;
    table[RegisterId::R11W] = 0b1011;
    table[RegisterId::R12W] = 0b1100;
    table[RegisterId::R13W] = 0b1101;
    table[RegisterId::R14W] = 0b1110;
    table[RegisterId::R15W] = 0b1111;
    table[RegisterId::AL] = 0b0000;
    table[RegisterId::CL] = 0b0001;
    table[RegisterId::DL] = 0b0010;
    table[RegisterId::BL] = 0b0011;
    table[RegisterId::SPL] = 0b0100;
    table[RegisterId::BPL] = 0b0101;
    table[RegisterId::SIL] = 0b0110;
    table[RegisterId::DIL] = 0b0111;
    table[RegisterId::AH] = 0b0100;
    table[RegisterId::CH] = 0b0101;
    table[RegisterId::DH] = 0b0110;
    table[RegisterId::BH] = 0b0111;
    table[RegisterId::R8B]  = 0b1000;
    table[RegisterId::R9B]  = 0b1001;
    table[RegisterId::R10B] = 0b1010;
    table[RegisterId::R11B] = 0b1011;
    table[RegisterId::R12B] = 0b1100;
    table[RegisterId::R13B] = 0b1101;
    table[RegisterId::R14B] = 0b1110;
    table[RegisterId::R15B] = 0b1111;
    table[RegisterId::XMM0] = 0;
    table[RegisterId::XMM1] = 1;
    table[RegisterId::XMM2] = 2;
    table[RegisterId::XMM3] = 3;
    table[RegisterId::XMM4] = 4;
    table[RegisterId::XMM5] = 5;
    table[RegisterId::XMM6] = 6;
    table[RegisterId::XMM7] = 7;
    table[RegisterId::XMM8] = 8;
    table[RegisterId::XMM9] = 9;
    table[RegisterId::XMM10] = 10;
    table[RegisterId::XMM11] = 11;
    table[RegisterId::XMM12] = 12;
    table[RegisterId::XMM13] = 13;
    table[RegisterId::XMM14] = 14;
    table[RegisterId::XMM15] = 15;
    return table;
}();

const std::array<InstructionEncoding, (size_t)Opcode::Count> s_instructionMapping = [] {
    std::array<InstructionEncoding, (size_t)Opcode::Count> table;

    // bytes, default64Bit, operandType, immediate, instructionVariant
    table[(size_t)Opcode::Ret] = InstructionEncoding({0xC3}, false, InstructionEncoding::None);

    table[(size_t)Opcode::Push64r] = InstructionEncoding({0x50}, true, InstructionEncoding::Embedded);
    table[(size_t)Opcode::Push32r] = InstructionEncoding({0x50}, true, InstructionEncoding::Embedded);
    table[(size_t)Opcode::Push16r] = InstructionEncoding({0x50}, true, InstructionEncoding::Embedded);

    table[(size_t)Opcode::Pop64r] = InstructionEncoding({0x58}, true, InstructionEncoding::Embedded);
    table[(size_t)Opcode::Pop32r] = InstructionEncoding({0x58}, true, InstructionEncoding::Embedded);
    table[(size_t)Opcode::Pop16r] = InstructionEncoding({0x58}, true, InstructionEncoding::Embedded);

    table[(size_t)Opcode::Mov64rr] = InstructionEncoding({0x89});
    table[(size_t)Opcode::Mov32rr] = InstructionEncoding({0x89});
    table[(size_t)Opcode::Mov16rr] = InstructionEncoding({0x89});
    table[(size_t)Opcode::Mov8rr] = InstructionEncoding({0x88});

    table[(size_t)Opcode::Movr64i64] = InstructionEncoding({0xB8}, false, InstructionEncoding::Embedded, true, ImmediateInt::imm64);
    table[(size_t)Opcode::Movr64i32] = InstructionEncoding({0xC7}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0);
    table[(size_t)Opcode::Mov32ri] = InstructionEncoding({0xB8}, false, InstructionEncoding::Embedded, true, ImmediateInt::imm32);
    table[(size_t)Opcode::Mov16ri] = InstructionEncoding({0xB8}, false, InstructionEncoding::Embedded, true, ImmediateInt::imm16);
    table[(size_t)Opcode::Mov8ri] = InstructionEncoding({0xB0}, false, InstructionEncoding::Embedded, true, ImmediateInt::imm8);

    table[(size_t)Opcode::Mov64rm] = InstructionEncoding({0x8B});
    table[(size_t)Opcode::Mov32rm] = InstructionEncoding({0x8B});
    table[(size_t)Opcode::Mov16rm] = InstructionEncoding({0x8B});
    table[(size_t)Opcode::Mov8rm] = InstructionEncoding({0x8A});

    table[(size_t)Opcode::Mov64mr] = InstructionEncoding({0x89});
    table[(size_t)Opcode::Mov32mr] = InstructionEncoding({0x89});
    table[(size_t)Opcode::Mov16mr] = InstructionEncoding({0x89});
    table[(size_t)Opcode::Mov8mr] = InstructionEncoding({0x88});

    table[(size_t)Opcode::Movm64i32] = InstructionEncoding({0xC7}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0);
    table[(size_t)Opcode::Mov32mi] = InstructionEncoding({0xC7}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0);
    table[(size_t)Opcode::Mov16mi] = InstructionEncoding({0xC7}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 0);
    table[(size_t)Opcode::Mov8mi] = InstructionEncoding({0xC6}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 0);

    table[(size_t)Opcode::Add64rr] = InstructionEncoding({0x01});
    table[(size_t)Opcode::Add32rr] = InstructionEncoding({0x01});
    table[(size_t)Opcode::Add16rr] = InstructionEncoding({0x01});
    table[(size_t)Opcode::Add8rr] = InstructionEncoding({0x00});

    table[(size_t)Opcode::Sub64rr] = InstructionEncoding({0x29});
    table[(size_t)Opcode::Sub32rr] = InstructionEncoding({0x29});
    table[(size_t)Opcode::Sub16rr] = InstructionEncoding({0x29});
    table[(size_t)Opcode::Sub8rr] = InstructionEncoding({0x28});

    table[(size_t)Opcode::Add64rm] = InstructionEncoding({0x03}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Add32rm] = InstructionEncoding({0x03}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Add16rm] = InstructionEncoding({0x03}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Add8rm] = InstructionEncoding({0x02}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Sub64rm] = InstructionEncoding({0x2B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Sub32rm] = InstructionEncoding({0x2B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Sub16rm] = InstructionEncoding({0x2B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Sub8rm] = InstructionEncoding({0x2A}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Add32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0x0);
    table[(size_t)Opcode::Add16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 0x0);
    table[(size_t)Opcode::Add8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, 1, 0x0);

    table[(size_t)Opcode::Sub32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0x5);
    table[(size_t)Opcode::Sub16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 0x5);
    table[(size_t)Opcode::Sub8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 0x5);

    table[(size_t)Opcode::Add64mr] = InstructionEncoding({0x01});
    table[(size_t)Opcode::Add32mr] = InstructionEncoding({0x01});
    table[(size_t)Opcode::Add16mr] = InstructionEncoding({0x01});
    table[(size_t)Opcode::Add8mr] = InstructionEncoding({0x00});

    table[(size_t)Opcode::Sub64mr] = InstructionEncoding({0x29});
    table[(size_t)Opcode::Sub32mr] = InstructionEncoding({0x29});
    table[(size_t)Opcode::Sub16mr] = InstructionEncoding({0x29});
    table[(size_t)Opcode::Sub8mr] = InstructionEncoding({0x28});

    table[(size_t)Opcode::Add32mi] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0x0);
    table[(size_t)Opcode::Add16mi] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 0x0);
    table[(size_t)Opcode::Add8mi] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 0x0);

    table[(size_t)Opcode::Sub32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0x5);
    table[(size_t)Opcode::Sub16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 0x5);
    table[(size_t)Opcode::Sub8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 0x5);

    table[(size_t)Opcode::Add64r32i] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0x0);
    table[(size_t)Opcode::Add64r8i] = InstructionEncoding({0x83}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 0x0);

    table[(size_t)Opcode::Sub64r32i] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 0x5);
    table[(size_t)Opcode::Sub64r8i] = InstructionEncoding({0x83}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 0x5);

    table[(size_t)Opcode::Jmp] = InstructionEncoding({0xE9}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Je] = InstructionEncoding({0x0F, 0x84}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jg] = InstructionEncoding({0x0F, 0x8F}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jge] = InstructionEncoding({0x0F, 0x8D}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jl] = InstructionEncoding({0x0F, 0x8C}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jle] = InstructionEncoding({0x0F, 0x8E}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jne] = InstructionEncoding({0x0F, 0x85}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jb] = InstructionEncoding({0x0F, 0x82}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jbe] = InstructionEncoding({0x0F, 0x86}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Ja] = InstructionEncoding({0x0F, 0x87}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Jae] = InstructionEncoding({0x0F, 0x83}, false, InstructionEncoding::Symbol);

    table[(size_t)Opcode::Cmp64rr] = InstructionEncoding({0x3B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmp32rr] = InstructionEncoding({0x3B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmp16rr] = InstructionEncoding({0x3B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmp8rr] = InstructionEncoding({0x3A}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Cmp64rm] = InstructionEncoding({0x3B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmp32rm] = InstructionEncoding({0x3B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmp16rm] = InstructionEncoding({0x3B}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmp8rm] = InstructionEncoding({0x3A}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Cmp32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 7);
    table[(size_t)Opcode::Cmp16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 7);
    table[(size_t)Opcode::Cmp8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 7);

    table[(size_t)Opcode::Lea64rm] = InstructionEncoding({0x8D}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Lea32rm] = InstructionEncoding({0x8D}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Lea16rm] = InstructionEncoding({0x8D}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Call] = InstructionEncoding({0xE8}, false, InstructionEncoding::Symbol);
    table[(size_t)Opcode::Call64r] = InstructionEncoding({0xFF}, false, InstructionEncoding::Normal, false, 0, 2);

    table[(size_t)Opcode::Rep_Movsb] = InstructionEncoding({0xF3, 0xA4}, false, InstructionEncoding::None);

    table[(size_t)Opcode::Movssrr] = InstructionEncoding({0xF3, 0x0F, 0x10}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movssrm] = InstructionEncoding({0xF3, 0x0F, 0x10}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movssmr] = InstructionEncoding({0xF3, 0x0F, 0x11});

    table[(size_t)Opcode::Addssrr] = InstructionEncoding({0xF3, 0x0F, 0x58}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Addssrm] = InstructionEncoding({0xF3, 0x0F, 0x58}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Addssmr] = InstructionEncoding({0xF3, 0x0F, 0x58}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Subssrr] = InstructionEncoding({0xF3, 0x0F, 0x5C}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Subssrm] = InstructionEncoding({0xF3, 0x0F, 0x5C}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Subssmr] = InstructionEncoding({0xF3, 0x0F, 0x5C}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Movsdrr] = InstructionEncoding({0xF2, 0x0F, 0x10}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsdrm] = InstructionEncoding({0xF2, 0x0F, 0x10}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsdmr] = InstructionEncoding({0xF2, 0x0F, 0x11}, true);

    table[(size_t)Opcode::Addsdrr] = InstructionEncoding({0xF2, 0x0F, 0x58}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Addsdrm] = InstructionEncoding({0xF2, 0x0F, 0x58}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Addsdmr] = InstructionEncoding({0xF2, 0x0F, 0x58}, true).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Subsdrr] = InstructionEncoding({0xF2, 0x0F, 0x5C}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Subsdrm] = InstructionEncoding({0xF2, 0x0F, 0x5C}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Subsdmr] = InstructionEncoding({0xF2, 0x0F, 0x5C}, true).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Movzx64r16r] = InstructionEncoding({0x0F, 0xB7}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx64r16m] = InstructionEncoding({0x0F, 0xB7}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx32r16r] = InstructionEncoding({0x0F, 0xB7}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx32r16m] = InstructionEncoding({0x0F, 0xB7}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx64r8r] = InstructionEncoding({0x0F, 0xB6}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx64r8m] = InstructionEncoding({0x0F, 0xB6}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx32r8r] = InstructionEncoding({0x0F, 0xB6}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx32r8m] = InstructionEncoding({0x0F, 0xB6}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx16r8r] = InstructionEncoding({0x0F, 0xB6}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movzx16r8m] = InstructionEncoding({0x0F, 0xB6}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx64r32r] = InstructionEncoding({0x63}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx64r32m] = InstructionEncoding({0x63}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx32r32r] = InstructionEncoding({0x63}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx32r32m] = InstructionEncoding({0x63}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx64r16r] = InstructionEncoding({0x0F, 0xBF}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx64r16m] = InstructionEncoding({0x0F, 0xBF}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx32r16r] = InstructionEncoding({0x0F, 0xBF}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx32r16m] = InstructionEncoding({0x0F, 0xBF}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx64r8r] = InstructionEncoding({0x0F, 0xBE}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx64r8m] = InstructionEncoding({0x0F, 0xBE}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx32r8r] = InstructionEncoding({0x0F, 0xBE}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx32r8m] = InstructionEncoding({0x0F, 0xBE}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx16r8r] = InstructionEncoding({0x0F, 0xBE}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movsx16r8m] = InstructionEncoding({0x0F, 0xBE}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtss2sdrr] = InstructionEncoding({0xF3, 0x0F, 0x5A}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtss2sdrm] = InstructionEncoding({0xF3, 0x0F, 0x5A}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsd2ssrr] = InstructionEncoding({0xF2, 0x0F, 0x5A}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsd2ssrm] = InstructionEncoding({0xF2, 0x0F, 0x5A}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsi2ss64rr] = InstructionEncoding({0xF3, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtsi2ss64rm] = InstructionEncoding({0xF3, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtsi2ss32rr] = InstructionEncoding({0xF3, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsi2ss32rm] = InstructionEncoding({0xF3, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsi2sd64rr] = InstructionEncoding({0xF2, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtsi2sd64rm] = InstructionEncoding({0xF2, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtsi2sd32rr] = InstructionEncoding({0xF2, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsi2sd32rm] = InstructionEncoding({0xF2, 0x0F, 0x2A}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsd2si64rr] = InstructionEncoding({0xF2, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtsd2si64rm] = InstructionEncoding({0xF2, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtsd2si32rr] = InstructionEncoding({0xF2, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtsd2si32rm] = InstructionEncoding({0xF2, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtss2si64rr] = InstructionEncoding({0xF3, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtss2si64rm] = InstructionEncoding({0xF3, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1).setRexPosition(1);
    table[(size_t)Opcode::Cvtss2si32rr] = InstructionEncoding({0xF3, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cvtss2si32rm] = InstructionEncoding({0xF3, 0x0F, 0x2D}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Shl8ri] = InstructionEncoding({0xC0}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 4);
    table[(size_t)Opcode::Shl8rCL] = InstructionEncoding({0xD2}, false, InstructionEncoding::Normal, false, 0, 4);
    table[(size_t)Opcode::Shl16ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 4);
    table[(size_t)Opcode::Shl16rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 4);
    table[(size_t)Opcode::Shl32ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 4);
    table[(size_t)Opcode::Shl32rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 4);
    table[(size_t)Opcode::Shl64ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 4);
    table[(size_t)Opcode::Shl64rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 4);

    table[(size_t)Opcode::Shr8ri] = InstructionEncoding({0xC0}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 5);
    table[(size_t)Opcode::Shr8rCL] = InstructionEncoding({0xD2}, false, InstructionEncoding::Normal, false, 0, 5);
    table[(size_t)Opcode::Shr16ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 5);
    table[(size_t)Opcode::Shr16rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 5);
    table[(size_t)Opcode::Shr32ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 5);
    table[(size_t)Opcode::Shr32rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 5);
    table[(size_t)Opcode::Shr64ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 5);
    table[(size_t)Opcode::Shr64rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 5);

    table[(size_t)Opcode::Sar8ri] = InstructionEncoding({0xC0}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 7);
    table[(size_t)Opcode::Sar8rCL] = InstructionEncoding({0xD2}, false, InstructionEncoding::Normal, false, 0, 7);
    table[(size_t)Opcode::Sar16ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 7);
    table[(size_t)Opcode::Sar16rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 7);
    table[(size_t)Opcode::Sar32ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 7);
    table[(size_t)Opcode::Sar32rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 7);
    table[(size_t)Opcode::Sar64ri] = InstructionEncoding({0xC1}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 7);
    table[(size_t)Opcode::Sar64rCL] = InstructionEncoding({0xD3}, false, InstructionEncoding::Normal, false, 0, 7);

    table[(size_t)Opcode::And8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 4);
    table[(size_t)Opcode::And8rr] = InstructionEncoding({0x20});
    table[(size_t)Opcode::And16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 4);
    table[(size_t)Opcode::And16rr] = InstructionEncoding({0x21});
    table[(size_t)Opcode::And32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 4);
    table[(size_t)Opcode::And32rr] = InstructionEncoding({0x21});
    table[(size_t)Opcode::And64rr] = InstructionEncoding({0x21});

    table[(size_t)Opcode::Or8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 1);
    table[(size_t)Opcode::Or8rr] = InstructionEncoding({0x08});
    table[(size_t)Opcode::Or16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 1);
    table[(size_t)Opcode::Or16rr] = InstructionEncoding({0x09});
    table[(size_t)Opcode::Or32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 1);
    table[(size_t)Opcode::Or32rr] = InstructionEncoding({0x09});
    table[(size_t)Opcode::Or64rr] = InstructionEncoding({0x09});

    table[(size_t)Opcode::Xor8ri] = InstructionEncoding({0x80}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8, 6);
    table[(size_t)Opcode::Xor8rr] = InstructionEncoding({0x30});
    table[(size_t)Opcode::Xor16ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm16, 6);
    table[(size_t)Opcode::Xor16rr] = InstructionEncoding({0x31});
    table[(size_t)Opcode::Xor32ri] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::imm32, 6);
    table[(size_t)Opcode::Xor32rr] = InstructionEncoding({0x31});
    table[(size_t)Opcode::Xor64rr] = InstructionEncoding({0x31});

    table[(size_t)Opcode::Cmpsdrr] = InstructionEncoding({0xF2, 0x0F, 0xC2}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Cmpssrr] = InstructionEncoding({0xF3, 0x0F, 0xC2}, false, InstructionEncoding::Normal, true, ImmediateInt::imm8).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Ucomisdrr] = InstructionEncoding({0x66, 0x0F, 0x2E}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Ucomissrr] = InstructionEncoding({0x0F, 0x2E}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Sete] = InstructionEncoding({0x0F, 0x94});
    table[(size_t)Opcode::Setg] = InstructionEncoding({0x0F, 0x9F});
    table[(size_t)Opcode::Setge] = InstructionEncoding({0x0F, 0x9D});
    table[(size_t)Opcode::Setl] = InstructionEncoding({0x0F, 0x9C});
    table[(size_t)Opcode::Setle] = InstructionEncoding({0x0F, 0x9E});
    table[(size_t)Opcode::Setne] = InstructionEncoding({0x0F, 0x95});
    table[(size_t)Opcode::Setb] = InstructionEncoding({0x0F, 0x92});
    table[(size_t)Opcode::Setbe] = InstructionEncoding({0x0F, 0x96});
    table[(size_t)Opcode::Seta] = InstructionEncoding({0x0F, 0x97});
    table[(size_t)Opcode::Setae] = InstructionEncoding({0x0F, 0x93});

    table[(size_t)Opcode::Cqo] = InstructionEncoding({0x99}, false, InstructionEncoding::None);
    table[(size_t)Opcode::Cdq] = InstructionEncoding({0x99}, false, InstructionEncoding::None);
    table[(size_t)Opcode::Cwd] = InstructionEncoding({0x99}, false, InstructionEncoding::None);
    table[(size_t)Opcode::Cbw] = InstructionEncoding({0x98}, false, InstructionEncoding::None);

    table[(size_t)Opcode::IDiv8] = InstructionEncoding({0xF6}, false, InstructionEncoding::Normal, false, 0, 7);
    table[(size_t)Opcode::IDiv16] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 7);
    table[(size_t)Opcode::IDiv32] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 7);
    table[(size_t)Opcode::IDiv64] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 7);

    table[(size_t)Opcode::Div8] = InstructionEncoding({0xF6}, false, InstructionEncoding::Normal, false, 0, 6);
    table[(size_t)Opcode::Div16] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 6);
    table[(size_t)Opcode::Div32] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 6);
    table[(size_t)Opcode::Div64] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 6);

    table[(size_t)Opcode::Divsdrr] = InstructionEncoding({0xF2, 0x0F, 0x5E}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Divssrr] = InstructionEncoding({0xF3, 0x0F, 0x5E}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Mul8] = InstructionEncoding({0xF6}, false, InstructionEncoding::Normal, false, 0, 4);
    table[(size_t)Opcode::Mul16] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 4);
    table[(size_t)Opcode::Mul32] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 4);
    table[(size_t)Opcode::Mul64] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 4);

    table[(size_t)Opcode::IMul16rr] = InstructionEncoding({0x0F, 0xAF}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::IMul32rr] = InstructionEncoding({0x0F, 0xAF}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::IMul64rr] = InstructionEncoding({0x0F, 0xAF}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::IMul64rri] = InstructionEncoding({0x69}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm64).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::IMul32rri] = InstructionEncoding({0x69}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm32).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::IMul16rri] = InstructionEncoding({0x69}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm16).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Mulssrr] = InstructionEncoding({0xF3, 0x0F, 0x59}).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Mulsdrr] = InstructionEncoding({0xF2, 0x0F, 0x59}).setRegIdx(0).setRmIdx(1);

    table[(size_t)Opcode::Cmp64r32i] = InstructionEncoding({0x81}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm32, 7);

    table[(size_t)Opcode::Jmp64r] = InstructionEncoding({0xFF}, false, InstructionEncoding::Normal, false, 0, 4);

    table[(size_t)Opcode::Test8rr] = InstructionEncoding({0x84});
    table[(size_t)Opcode::Test16rr] = InstructionEncoding({0x85});
    table[(size_t)Opcode::Test32rr] = InstructionEncoding({0x85});
    table[(size_t)Opcode::Test64rr] = InstructionEncoding({0x85});

    table[(size_t)Opcode::Not64r] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 2);
    table[(size_t)Opcode::Not32r] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 2);
    table[(size_t)Opcode::Not16r] = InstructionEncoding({0xF7}, false, InstructionEncoding::Normal, false, 0, 2);
    table[(size_t)Opcode::Not8r] = InstructionEncoding({0xF6}, false, InstructionEncoding::Normal, false, 0, 2);

    table[(size_t)Opcode::Movapsrr] = InstructionEncoding({0x0F, 0x28}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movapsrm] = InstructionEncoding({0x0F, 0x28}, true).setRegIdx(0).setRmIdx(1);
    table[(size_t)Opcode::Movapsmr] = InstructionEncoding({0x0F, 0x29}, true).setRegIdx(1).setRmIdx(0);

    table[(size_t)Opcode::Push32i] = InstructionEncoding({0x68}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm32);
    table[(size_t)Opcode::Push16i] = InstructionEncoding({0x68}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm16);
    table[(size_t)Opcode::Push8i] = InstructionEncoding({0x6A}, false, InstructionEncoding::Normal, true, ImmediateInt::Size::imm8);

    return table;
}();

}