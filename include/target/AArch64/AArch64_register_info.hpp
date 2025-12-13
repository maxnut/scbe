#pragma once

#include "target/register_info.hpp"

#include <cstdint>

namespace scbe::Target::AArch64 {

enum RegisterId : uint32_t {
    // General-purpose registers (64-bit)
    X0 = 0, X1, X2, X3, X4, X5, X6, X7,
    X8, X9, X10, X11, X12, X13, X14, X15,
    X16, X17, X18, X19, X20, X21, X22, X23,
    X24, X25, X26, X27, X28, X29, X30, XZR,

    // 32-bit aliases (W0–W30)
    W0, W1, W2, W3, W4, W5, W6, W7,
    W8, W9, W10, W11, W12, W13, W14, W15,
    W16, W17, W18, W19, W20, W21, W22, W23,
    W24, W25, W26, W27, W28, W29, W30, WZR,

    // Stack pointer (pseudo-register)
    SP,

    // SIMD/Floating-point registers (128-bit Q)
    Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7,
    Q8, Q9, Q10, Q11, Q12, Q13, Q14, Q15,
    Q16, Q17, Q18, Q19, Q20, Q21, Q22, Q23,
    Q24, Q25, Q26, Q27, Q28, Q29, Q30, Q31,

    // Aliases to 64-bit FP scalar (D0–D31)
    D0, D1, D2, D3, D4, D5, D6, D7,
    D8, D9, D10, D11, D12, D13, D14, D15,
    D16, D17, D18, D19, D20, D21, D22, D23,
    D24, D25, D26, D27, D28, D29, D30, D31,

    // Aliases to 32-bit FP scalar (S0–S31)
    S0, S1, S2, S3, S4, S5, S6, S7,
    S8, S9, S10, S11, S12, S13, S14, S15,
    S16, S17, S18, S19, S20, S21, S22, S23,
    S24, S25, S26, S27, S28, S29, S30, S31,

    Count
};

enum SubRegIndex : uint32_t {
    Sub32 = 0, // Xn → Wn
    Sub64 = 1, // Qn → Dn
};

enum RegisterClass : uint32_t {
    GPR64 = 0,
    GPR32,
    FPR128, // Qn
    FPR64,   // Dn
    FPR32   // Sn
};

class AArch64RegisterInfo : public RegisterInfo {
public:
    AArch64RegisterInfo();

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
