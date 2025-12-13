#pragma once

#include "target/register_info.hpp"

#include <cstdint>

namespace scbe::Target::x64 {

enum RegisterId : uint32_t {
    // 64-bit registers
    RAX = 0, RBX, RCX, RDX,
    RSI, RDI, RBP, RSP,
    R8,  R9,  R10, R11,
    R12, R13, R14, R15,

    // 32-bit subregisters
    EAX, EBX, ECX, EDX,
    ESI, EDI, EBP, ESP,
    R8D, R9D, R10D, R11D,
    R12D, R13D, R14D, R15D,

    // 16-bit subregisters
    AX, BX, CX, DX,
    SI, DI, BP, SP,
    R8W, R9W, R10W, R11W,
    R12W, R13W, R14W, R15W,

    // 8-bit subregisters (low)
    AL, BL, CL, DL,
    SIL, DIL, BPL, SPL,
    R8B, R9B, R10B, R11B,
    R12B, R13B, R14B, R15B,

    // 8-bit subregisters (high, legacy only)
    AH, BH, CH, DH,

    // Floating point registers
    XMM0, XMM1, XMM2, XMM3,
    XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11,
    XMM12, XMM13, XMM14, XMM15,

    RIP,

    Count
};

enum SubRegIndex : uint32_t {
    Sub8Bit = 0,
    Sub8BitHi = 1,
    Sub16Bit = 2,
    Sub32Bit = 3
};

enum RegisterClass : uint32_t {
    GPR64 = 0,
    GPR32,
    GPR16,
    GPR8,
    FPR
};

class x64RegisterInfo : public RegisterInfo {
public:
    x64RegisterInfo();

    uint32_t getSubReg(uint32_t reg, uint32_t idx) override;
    uint32_t getClassFromType(Type* type) override;
    const std::vector<uint32_t>& getCallerSavedRegisters() const override;
    const std::vector<uint32_t>& getCalleeSavedRegisters() const override;
    const std::vector<uint32_t>& getReservedRegisters(uint32_t rclass) const override;
    const std::vector<uint32_t>& getAvailableRegisters(uint32_t rclass) const override;
    const std::vector<uint32_t>& getReservedRegistersCanonical(uint32_t rclass) const override;
    const std::vector<uint32_t>& getAvailableRegistersCanonical(uint32_t rclass) const override;
    bool doClassesOverlap(uint32_t class1, uint32_t class2) const override;
};

}