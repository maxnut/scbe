#include "target/AArch64/AArch64_register_info.hpp"
#include "cast.hpp"
#include "type.hpp"
#include <cassert>

namespace scbe::Target::AArch64 {

AArch64RegisterInfo::AArch64RegisterInfo() {
    m_registerDescs = {
        { "x0",  {}, {W0}, {W0}, GPR64 },
        { "x1",  {}, {W1}, {W1}, GPR64 },
        { "x2",  {}, {W2}, {W2}, GPR64 },
        { "x3",  {}, {W3}, {W3}, GPR64 },
        { "x4",  {}, {W4}, {W4}, GPR64 },
        { "x5",  {}, {W5}, {W5}, GPR64 },
        { "x6",  {}, {W6}, {W6}, GPR64 },
        { "x7",  {}, {W7}, {W7}, GPR64 },
        { "x8",  {}, {W8}, {W8}, GPR64 },
        { "x9",  {}, {W9}, {W9}, GPR64 },
        { "x10", {}, {W10}, {W10}, GPR64 },
        { "x11", {}, {W11}, {W11}, GPR64 },
        { "x12", {}, {W12}, {W12}, GPR64 },
        { "x13", {}, {W13}, {W13}, GPR64 },
        { "x14", {}, {W14}, {W14}, GPR64 },
        { "x15", {}, {W15}, {W15}, GPR64 },
        { "x16", {}, {W16}, {W16}, GPR64 },
        { "x17", {}, {W17}, {W17}, GPR64 },
        { "x18", {}, {W18}, {W18}, GPR64 },
        { "x19", {}, {W19}, {W19}, GPR64 },
        { "x20", {}, {W20}, {W20}, GPR64 },
        { "x21", {}, {W21}, {W21}, GPR64 },
        { "x22", {}, {W22}, {W22}, GPR64 },
        { "x23", {}, {W23}, {W23}, GPR64 },
        { "x24", {}, {W24}, {W24}, GPR64 },
        { "x25", {}, {W25}, {W25}, GPR64 },
        { "x26", {}, {W26}, {W26}, GPR64 },
        { "x27", {}, {W27}, {W27}, GPR64 },
        { "x28", {}, {W28}, {W28}, GPR64 },
        { "x29", {}, {W29}, {W29}, GPR64 },
        { "x30", {}, {W30}, {W30}, GPR64 },
        { "xzr", {}, {WZR}, {WZR}, GPR64 },

        { "w0",  {X0}, {}, {X0}, GPR32 },
        { "w1",  {X1}, {}, {X1}, GPR32 },
        { "w2",  {X2}, {}, {X2}, GPR32 },
        { "w3",  {X3}, {}, {X3}, GPR32 },
        { "w4",  {X4}, {}, {X4}, GPR32 },
        { "w5",  {X5}, {}, {X5}, GPR32 },
        { "w6",  {X6}, {}, {X6}, GPR32 },
        { "w7",  {X7}, {}, {X7}, GPR32 },
        { "w8",  {X8}, {}, {X8}, GPR32 },
        { "w9",  {X9}, {}, {X9}, GPR32 },
        { "w10", {X10}, {}, {X10}, GPR32 },
        { "w11", {X11}, {}, {X11}, GPR32 },
        { "w12", {X12}, {}, {X12}, GPR32 },
        { "w13", {X13}, {}, {X13}, GPR32 },
        { "w14", {X14}, {}, {X14}, GPR32 },
        { "w15", {X15}, {}, {X15}, GPR32 },
        { "w16", {X16}, {}, {X16}, GPR32 },
        { "w17", {X17}, {}, {X17}, GPR32 },
        { "w18", {X18}, {}, {X18}, GPR32 },
        { "w19", {X19}, {}, {X19}, GPR32 },
        { "w20", {X20}, {}, {X20}, GPR32 },
        { "w21", {X21}, {}, {X21}, GPR32 },
        { "w22", {X22}, {}, {X22}, GPR32 },
        { "w23", {X23}, {}, {X23}, GPR32 },
        { "w24", {X24}, {}, {X24}, GPR32 },
        { "w25", {X25}, {}, {X25}, GPR32 },
        { "w26", {X26}, {}, {X26}, GPR32 },
        { "w27", {X27}, {}, {X27}, GPR32 },
        { "w28", {X28}, {}, {X28}, GPR32 },
        { "w29", {X29}, {}, {X29}, GPR32 },
        { "w30", {X30}, {}, {X30}, GPR32 },
        { "wzr", {XZR}, {}, {XZR}, GPR32 },

        { "sp", {}, {}, {}, GPR64 },

        // Vector registers
        { "q0", {}, {D0, S0}, {D0, S0}, FPR128 },
        { "q1", {}, {D1, S1}, {D1, S1}, FPR128 },
        { "q2", {}, {D2, S2}, {D2, S2}, FPR128 },
        { "q3", {}, {D3, S3}, {D3, S3}, FPR128 },
        { "q4", {}, {D4, S4}, {D4, S4}, FPR128 },
        { "q5", {}, {D5, S5}, {D5, S5}, FPR128 },
        { "q6", {}, {D6, S6}, {D6, S6}, FPR128 },
        { "q7", {}, {D7, S7}, {D7, S7}, FPR128 },
        { "q8", {}, {D8, S8}, {D8, S8}, FPR128 },
        { "q9", {}, {D9, S9}, {D9, S9}, FPR128 },
        { "q10", {}, {D10, S10}, {D10, S10}, FPR128 },
        { "q11", {}, {D11, S11}, {D11, S11}, FPR128 },
        { "q12", {}, {D12, S12}, {D12, S12}, FPR128 },
        { "q13", {}, {D13, S13}, {D13, S13}, FPR128 },
        { "q14", {}, {D14, S14}, {D14, S14}, FPR128 },
        { "q15", {}, {D15, S15}, {D15, S15}, FPR128 },
        { "q16", {}, {D16, S16}, {D16, S16}, FPR128 },
        { "q17", {}, {D17, S17}, {D17, S17}, FPR128 },
        { "q18", {}, {D18, S18}, {D18, S18}, FPR128 },
        { "q19", {}, {D19, S19}, {D19, S19}, FPR128 },
        { "q20", {}, {D20, S20}, {D20, S20}, FPR128 },
        { "q21", {}, {D21, S21}, {D21, S21}, FPR128 },
        { "q22", {}, {D22, S22}, {D22, S22}, FPR128 },
        { "q23", {}, {D23, S23}, {D23, S23}, FPR128 },
        { "q24", {}, {D24, S24}, {D24, S24}, FPR128 },
        { "q25", {}, {D25, S25}, {D25, S25}, FPR128 },
        { "q26", {}, {D26, S26}, {D26, S26}, FPR128 },
        { "q27", {}, {D27, S27}, {D27, S27}, FPR128 },
        { "q28", {}, {D28, S28}, {D28, S28}, FPR128 },
        { "q29", {}, {D29, S29}, {D29, S29}, FPR128 },
        { "q30", {}, {D30, S30}, {D30, S30}, FPR128 },
        { "q31", {}, {D31, S31}, {D31, S31}, FPR128 },

        { "d0", {Q0}, {S0}, {Q0,S0}, FPR64 },
        { "d1", {Q1}, {S1}, {Q1,S1}, FPR64 },
        { "d2", {Q2}, {S2}, {Q2,S2}, FPR64 },
        { "d3", {Q3}, {S3}, {Q3,S3}, FPR64 },
        { "d4", {Q4}, {S4}, {Q4,S4}, FPR64 },
        { "d5", {Q5}, {S5}, {Q5,S5}, FPR64 },
        { "d6", {Q6}, {S6}, {Q6,S6}, FPR64 },
        { "d7", {Q7}, {S7}, {Q7,S7}, FPR64 },
        { "d8", {Q8}, {S8}, {Q8,S8}, FPR64 },
        { "d9", {Q9}, {S9}, {Q9,S9}, FPR64 },
        { "d10", {Q10}, {S10}, {Q10,S10}, FPR64 },
        { "d11", {Q11}, {S11}, {Q11,S11}, FPR64 },
        { "d12", {Q12}, {S12}, {Q12,S12}, FPR64 },
        { "d13", {Q13}, {S13}, {Q13,S13}, FPR64 },
        { "d14", {Q14}, {S14}, {Q14,S14}, FPR64 },
        { "d15", {Q15}, {S15}, {Q15,S15}, FPR64 },
        { "d16", {Q16}, {S16}, {Q16,S16}, FPR64 },
        { "d17", {Q17}, {S17}, {Q17,S17}, FPR64 },
        { "d18", {Q18}, {S18}, {Q18,S18}, FPR64 },
        { "d19", {Q19}, {S19}, {Q19,S19}, FPR64 },
        { "d20", {Q20}, {S20}, {Q20,S20}, FPR64 },
        { "d21", {Q21}, {S21}, {Q21,S21}, FPR64 },
        { "d22", {Q22}, {S22}, {Q22,S22}, FPR64 },
        { "d23", {Q23}, {S23}, {Q23,S23}, FPR64 },
        { "d24", {Q24}, {S24}, {Q24,S24}, FPR64 },
        { "d25", {Q25}, {S25}, {Q25,S25}, FPR64 },
        { "d26", {Q26}, {S26}, {Q26,S26}, FPR64 },
        { "d27", {Q27}, {S27}, {Q27,S27}, FPR64 },
        { "d28", {Q28}, {S28}, {Q28,S28}, FPR64 },
        { "d29", {Q29}, {S29}, {Q29,S29}, FPR64 },
        { "d30", {Q30}, {S30}, {Q30,S30}, FPR64 },
        { "d31", {Q31}, {S31}, {Q31,S31}, FPR64 },

        { "s0", {D0, Q0}, {}, {D0, Q0}, FPR32 },
        { "s1", {D1, Q1}, {}, {D1, Q1}, FPR32 },
        { "s2", {D2, Q2}, {}, {D2, Q2}, FPR32 },
        { "s3", {D3, Q3}, {}, {D3, Q3}, FPR32 },
        { "s4", {D4, Q4}, {}, {D4, Q4}, FPR32 },
        { "s5", {D5, Q5}, {}, {D5, Q5}, FPR32 },
        { "s6", {D6, Q6}, {}, {D6, Q6}, FPR32 },
        { "s7", {D7, Q7}, {}, {D7, Q7}, FPR32 },
        { "s8", {D8, Q8}, {}, {D8, Q8}, FPR32 },
        { "s9", {D9, Q9}, {}, {D9, Q9}, FPR32 },
        { "s10", {D10, Q10}, {}, {D10, Q10}, FPR32 },
        { "s11", {D11, Q11}, {}, {D11, Q11}, FPR32 },
        { "s12", {D12, Q12}, {}, {D12, Q12}, FPR32 },
        { "s13", {D13, Q13}, {}, {D13, Q13}, FPR32 },
        { "s14", {D14, Q14}, {}, {D14, Q14}, FPR32 },
        { "s15", {D15, Q15}, {}, {D15, Q15}, FPR32 },
        { "s16", {D16, Q16}, {}, {D16, Q16}, FPR32 },
        { "s17", {D17, Q17}, {}, {D17, Q17}, FPR32 },
        { "s18", {D18, Q18}, {}, {D18, Q18}, FPR32 },
        { "s19", {D19, Q19}, {}, {D19, Q19}, FPR32 },
        { "s20", {D20, Q20}, {}, {D20, Q20}, FPR32 },
        { "s21", {D21, Q21}, {}, {D21, Q21}, FPR32 },
        { "s22", {D22, Q22}, {}, {D22, Q22}, FPR32 },
        { "s23", {D23, Q23}, {}, {D23, Q23}, FPR32 },
        { "s24", {D24, Q24}, {}, {D24, Q24}, FPR32 },
        { "s25", {D25, Q25}, {}, {D25, Q25}, FPR32 },
        { "s26", {D26, Q26}, {}, {D26, Q26}, FPR32 },
        { "s27", {D27, Q27}, {}, {D27, Q27}, FPR32 },
        { "s28", {D28, Q28}, {}, {D28, Q28}, FPR32 },
        { "s29", {D29, Q29}, {}, {D29, Q29}, FPR32 },
        { "s30", {D30, Q30}, {}, {D30, Q30}, FPR32 },
        { "s31", {D31, Q31}, {}, {D31, Q31}, FPR32 },
    };

    m_registerClasses = {
        {{X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15, X16, X17, X18, X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, X29, X30, SP, XZR}, 8, 8},
        {{W0, W1, W2, W3, W4, W5, W6, W7, W8, W9, W10, W11, W12, W13, W14, W15, W16, W17, W18, W19, W20, W21, W22, W23, W24, W25, W26, W27, W28, W29, W30, WZR}, 4, 4},
        {{Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, Q10, Q11, Q12, Q13, Q14, Q15, Q16, Q17, Q18, Q19, Q20, Q21, Q22, Q23, Q24, Q25, Q26, Q27, Q28, Q29, Q30, Q31}, 16, 16},
        {{D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18, D19, D20, D21, D22, D23, D24, D25, D26, D27, D28, D29, D30, D31}, 8, 8},
        {{S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, S12, S13, S14, S15, S16, S17, S18, S19, S20, S21, S22, S23, S24, S25, S26, S27, S28, S29, S30, S31}, 4, 4},
    };

    m_subRegIndexDescs = {
        {"Sub32", 0, 32},
        {"Sub64", 0, 64},
        
    };
}

static const uint32_t s_subRegTable[Count][2] = {

    /* X0  */ { W0,   X0 },
    /* X1  */ { W1,   X1 },
    /* X2  */ { W2,   X2 },
    /* X3  */ { W3,   X3 },
    /* X4  */ { W4,   X4 },
    /* X5  */ { W5,   X5 },
    /* X6  */ { W6,   X6 },
    /* X7  */ { W7,   X7 },
    /* X8  */ { W8,   X8 },
    /* X9  */ { W9,   X9 },
    /* X10 */ { W10,  X10 },
    /* X11 */ { W11,  X11 },
    /* X12 */ { W12,  X12 },
    /* X13 */ { W13,  X13 },
    /* X14 */ { W14,  X14 },
    /* X15 */ { W15,  X15 },
    /* X16 */ { W16,  X16 },
    /* X17 */ { W17,  X17 },
    /* X18 */ { W18,  X18 },
    /* X19 */ { W19,  X19 },
    /* X20 */ { W20,  X20 },
    /* X21 */ { W21,  X21 },
    /* X22 */ { W22,  X22 },
    /* X23 */ { W23,  X23 },
    /* X24 */ { W24,  X24 },
    /* X25 */ { W25,  X25 },
    /* X26 */ { W26,  X26 },
    /* X27 */ { W27,  X27 },
    /* X28 */ { W28,  X28 },
    /* FP  */ { W29,  X29 },
    /* WZR  */ { WZR,  XZR },
};

static const std::vector<uint32_t> s_callerSavedRegisters = {
    X0, X1, X2, X3, X4, X5, X6, X7,
    X9, X10, X11, X12, X13, X14, X15
};

static const std::vector<uint32_t> s_calleeSavedRegisters = {
    X19, X20, X21, X22, X23, X24, X25, X26, X27, X28,
};

static const std::vector<uint32_t> s_reservedRegistersGpr64 = {
    X14, X15
};
static const std::vector<uint32_t> s_reservedRegistersGpr32 = {
    W14, W15
};
static const std::vector<uint32_t> s_reservedRegistersFpr32 = {
    S30, S31
};
static const std::vector<uint32_t> s_reservedRegistersFpr64 = {
    D30, D31
};
static const std::vector<uint32_t> s_reservedRegistersFpr128 = {
    Q30, Q31
};
static const std::vector<uint32_t> s_availableRegistersGpr64 = {
    X0, X1, X2, X3, X4, X5, X6, X7,
    X8, X9, X10, X11, X12, X13,
    X16, X17,
    X18,
    X19, X20, X21, X22, X23, X24, X25, X26, X27, X28
};

static const std::vector<uint32_t> s_availableRegistersGpr32 = {
    W0, W1, W2, W3, W4, W5, W6, W7,
    W8, W9, W10, W11, W12, W13,
    W16, W17,
    W18,
    W19, W20, W21, W22, W23, W24, W25, W26, W27, W28
};
static const std::vector<uint32_t> s_availableRegistersFpr32 = {
    S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11, S12, S13, S14, S15,
    S16, S17, S18, S19, S20, S21, S22, S23, S24, S25, S26, S27, S28, S29,
};
static const std::vector<uint32_t> s_availableRegistersFpr64 = {
    D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15,
    D16, D17, D18, D19, D20, D21, D22, D23, D24, D25, D26, D27, D28, D29,
};
static const std::vector<uint32_t> s_availableRegistersFpr128 = {
    Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, Q10, Q11, Q12, Q13, Q14, Q15,
    Q16, Q17, Q18, Q19, Q20, Q21, Q22, Q23, Q24, Q25, Q26, Q27, Q28, Q29,
};

uint32_t AArch64RegisterInfo::getSubReg(uint32_t reg, uint32_t idx) {
    return s_subRegTable[reg][idx];
}

uint32_t AArch64RegisterInfo::getClassFromType(Type* type) {
    switch(type->getKind()) {
        case Type::TypeKind::Integer: {
            IntegerType* itype = (IntegerType*)(type);
            return itype->getBits() == 64 ? GPR64 : GPR32;
        }
        case Type::TypeKind::Float: {
            FloatType* ftype = (FloatType*)(type);
            return ftype->getBits() == 32 ? FPR32 : FPR64;
        }
        case Type::TypeKind::Pointer:
            return GPR64;
        case Type::TypeKind::Void:
        case Type::TypeKind::Struct:
        case Type::TypeKind::Array:
        case Type::TypeKind::Function:
            break;
    }
    assert(false && "Should be unreachable!");
    return GPR64;
}
    
const std::vector<uint32_t>& AArch64RegisterInfo::getCallerSavedRegisters() const {
    return s_callerSavedRegisters;
}

const std::vector<uint32_t>& AArch64RegisterInfo::getCalleeSavedRegisters() const {
    return s_calleeSavedRegisters;
}

const std::vector<uint32_t>& AArch64RegisterInfo::getReservedRegisters(uint32_t rclass) const {
    switch(rclass) {
        case GPR64: return s_reservedRegistersGpr64;
        case GPR32: return s_reservedRegistersGpr32;
        case FPR32:   return s_reservedRegistersFpr32;
        case FPR64:   return s_reservedRegistersFpr64;
        case FPR128:   return s_reservedRegistersFpr128;
        default: break;
    }
    return s_reservedRegistersGpr64;
}

const std::vector<uint32_t>& AArch64RegisterInfo::getAvailableRegisters(uint32_t rclass) const {
    switch(rclass) {
        case GPR64: return s_availableRegistersGpr64;
        case GPR32: return s_availableRegistersGpr32;
        case FPR32:   return s_availableRegistersFpr32;
        case FPR64:   return s_availableRegistersFpr64;
        case FPR128:   return s_availableRegistersFpr128;
        default: break;
    }
    return s_availableRegistersGpr64;
}

const std::vector<uint32_t>& AArch64RegisterInfo::getReservedRegistersCanonical(uint32_t rclass) const {
    switch(rclass) {
        case GPR64:
        case GPR32: return s_reservedRegistersGpr64;
        case FPR32:
        case FPR64:
        case FPR128:   return s_reservedRegistersFpr128;
        default: break;
    }
    return s_reservedRegistersGpr64;
}

const std::vector<uint32_t>& AArch64RegisterInfo::getAvailableRegistersCanonical(uint32_t rclass) const {
    switch(rclass) {
        case GPR64:
        case GPR32: return s_availableRegistersGpr64;
        case FPR32:
        case FPR64:
        case FPR128:   return s_availableRegistersFpr128;
        default: break;
    }
    return s_availableRegistersGpr64;
}

bool AArch64RegisterInfo::doClassesOverlap(uint32_t class1, uint32_t class2) const {
    return class1 == class2 ||
        ((class1 >= GPR64 && class1 <= GPR32) && (class2 >= GPR64 && class2 <= GPR32)) ||
        ((class1 >= FPR128 && class1 <= FPR32) && (class2 >= FPR128 && class2 <= FPR32));
}

bool AArch64RegisterInfo::isFPR(uint32_t rclass) const {
    return rclass == FPR32 || rclass == FPR64;
}

}