#include "target/x64/x64_register_info.hpp"
#include "type.hpp"
#include <cassert>
#include <vector>

namespace scbe::Target::x64 {

x64RegisterInfo::x64RegisterInfo() {
    m_registerDescs = {
        {"rax", {}, {EAX, AX, AH, AL}, {EAX, AX, AH, AL}, GPR64},
        {"rbx", {}, {EBX, BX, BH, BL}, {EBX, BX, BH, BL}, GPR64},
        {"rcx", {}, {ECX, CX, CH, CL}, {ECX, CX, CH, CL}, GPR64},
        {"rdx", {}, {EDX, DX, DH, DL}, {EDX, DX, DH, DL}, GPR64},
        {"rsi", {}, {ESI, SI, SIL}, {ESI, SI, SIL}, GPR64},
        {"rdi", {}, {EDI, DI, DIL}, {EDI, DI, DIL}, GPR64},
        {"rbp", {}, {EBP, BP, BPL}, {EBP, BP, BPL}, GPR64},
        {"rsp", {}, {ESP, SP, SPL}, {ESP, SP, SPL}, GPR64},
        {"r8",  {}, {R8D, R8W, R8B}, {R8D, R8W, R8B}, GPR64},
        {"r9",  {}, {R9D, R9W, R9B}, {R9D, R9W, R9B}, GPR64},
        {"r10", {}, {R10D, R10W, R10B}, {R10D, R10W, R10B}, GPR64},
        {"r11", {}, {R11D, R11W, R11B}, {R11D, R11W, R11B}, GPR64},
        {"r12", {}, {R12D, R12W, R12B}, {R12D, R12W, R12B}, GPR64},
        {"r13", {}, {R13D, R13W, R13B}, {R13D, R13W, R13B}, GPR64},
        {"r14", {}, {R14D, R14W, R14B}, {R14D, R14W, R14B}, GPR64},
        {"r15", {}, {R15D, R15W, R15B}, {R15D, R15W, R15B}, GPR64},

        {"eax", {RAX}, {AX, AH, AL}, {RAX, AX, AH, AL}, GPR32},
        {"ebx", {RBX}, {BX, BH, BL}, {RBX, BX, BH, BL}, GPR32},
        {"ecx", {RCX}, {CX, CH, CL}, {RCX, CX, CH, CL}, GPR32},
        {"edx", {RDX}, {DX, DH, DL}, {RDX, DX, DH, DL}, GPR32},
        {"esi", {RSI}, {SI, SIL}, {RSI, SI, SIL}, GPR32},
        {"edi", {RDI}, {DI, DIL}, {RDI, DI, DIL}, GPR32},
        {"ebp", {RBP}, {BP, BPL}, {RBP, BP, BPL}, GPR32},
        {"esp", {RSP}, {SP, SPL}, {RSP, SP, SPL}, GPR32},
        {"r8d",  {R8},  {R8W, R8B}, {R8, R8W, R8B}, GPR32},
        {"r9d",  {R9},  {R9W, R9B}, {R9, R9W, R9B}, GPR32},
        {"r10d", {R10}, {R10W, R10B}, {R10, R10W, R10B}, GPR32},
        {"r11d", {R11}, {R11W, R11B}, {R11, R11W, R11B}, GPR32},
        {"r12d", {R12}, {R12W, R12B}, {R12, R12W, R12B}, GPR32},
        {"r13d", {R13}, {R13W, R13B}, {R13, R13W, R13B}, GPR32},
        {"r14d", {R14}, {R14W, R14B}, {R14, R14W, R14B}, GPR32},
        {"r15d", {R15}, {R15W, R15B}, {R15, R15W, R15B}, GPR32},

        {"ax", {RAX, EAX}, {AL, AH}, {RAX, EAX, AL, AH}, GPR16},
        {"bx", {RBX, EBX}, {BL, BH}, {RBX, EBX, BL, BH}, GPR16},
        {"cx", {RCX, ECX}, {CL, CH}, {RCX, ECX, CL, CH}, GPR16},
        {"dx", {RDX, EDX}, {DL, DH}, {RDX, EDX, DL, DH}, GPR16},
        {"si", {RSI, ESI}, {SIL}, {RSI, ESI, SIL}, GPR16},
        {"di", {RDI, EDI}, {DIL}, {RDI, EDI, DIL}, GPR16},
        {"bp", {RBP, EBP}, {BPL}, {RBP, EBP, BPL}, GPR16},
        {"sp", {RSP, ESP}, {SPL}, {RSP, ESP, SPL}, GPR16},
        {"r8w",  {R8, R8D},  {R8B},  {R8, R8D, R8B}, GPR16},
        {"r9w",  {R9, R9D},  {R9B},  {R9, R9D, R9B}, GPR16},
        {"r10w", {R10, R10D}, {R10B}, {R10, R10D, R10B}, GPR16},
        {"r11w", {R11, R11D}, {R11B}, {R11, R11D, R11B}, GPR16},
        {"r12w", {R12, R12D}, {R12B}, {R12, R12D, R12B}, GPR16},
        {"r13w", {R13, R13D}, {R13B}, {R13, R13D, R13B}, GPR16},
        {"r14w", {R14, R14D}, {R14B}, {R14, R14D, R14B}, GPR16},
        {"r15w", {R15, R15D}, {R15B}, {R15, R15D, R15B}, GPR16},

        {"al", {RAX, EAX, AX}, {}, {RAX, EAX, AX, AH}, GPR8},
        {"bl", {RBX, EBX, BX}, {}, {RBX, EBX, BX, BH}, GPR8},
        {"cl", {RCX, ECX, CX}, {}, {RCX, ECX, CX, CH}, GPR8},
        {"dl", {RDX, EDX, DX}, {}, {RDX, EDX, DX, DH}, GPR8},
        {"sil", {RSI, ESI, SI}, {}, {RSI, ESI, SI}, GPR8},
        {"dil", {RDI, EDI, DI}, {}, {RDI, EDI, DI}, GPR8},
        {"bpl", {RBP, EBP, BP}, {}, {RBP, EBP, BP}, GPR8},
        {"spl", {RSP, ESP, SP}, {}, {RSP, ESP, SP}, GPR8},
        {"r8b",  {R8, R8D, R8W},  {},  {R8, R8D, R8W}, GPR8},
        {"r9b",  {R9, R9D, R9W},  {},  {R9, R9D, R9W}, GPR8},
        {"r10b", {R10, R10D, R10W}, {}, {R10, R10D, R10W}, GPR8},
        {"r11b", {R11, R11D, R11W}, {}, {R11, R11D, R11W}, GPR8},
        {"r12b", {R12, R12D, R12W}, {}, {R12, R12D, R12W}, GPR8},
        {"r13b", {R13, R13D, R13W}, {}, {R13, R13D, R13W}, GPR8},
        {"r14b", {R14, R14D, R14W}, {}, {R14, R14D, R14W}, GPR8},
        {"r15b", {R15, R15D, R15W}, {}, {R15, R15D, R15W}, GPR8},

        {"ah", {RAX, EAX, AX}, {}, {RAX, EAX, AX, AL}, GPR8},
        {"bh", {RBX, EBX, BX}, {}, {RBX, EBX, BX, AL}, GPR8},
        {"ch", {RCX, ECX, CX}, {}, {RCX, ECX, CX, AL}, GPR8},
        {"dh", {RDX, EDX, DX}, {}, {RDX, EDX, DX, AL}, GPR8},

        {"xmm0", {}, {}, {}, FPR},
        {"xmm1", {}, {}, {}, FPR},
        {"xmm2", {}, {}, {}, FPR},
        {"xmm3", {}, {}, {}, FPR},
        {"xmm4", {}, {}, {}, FPR},
        {"xmm5", {}, {}, {}, FPR},
        {"xmm6", {}, {}, {}, FPR},
        {"xmm7", {}, {}, {}, FPR},
        {"xmm8", {}, {}, {}, FPR},
        {"xmm9", {}, {}, {}, FPR},
        {"xmm10", {}, {}, {}, FPR},
        {"xmm11", {}, {}, {}, FPR},
        {"xmm12", {}, {}, {}, FPR},
        {"xmm13", {}, {}, {}, FPR},
        {"xmm14", {}, {}, {}, FPR},
        {"xmm15", {}, {}, {}, FPR},

        {"rip", {}, {}, {}, GPR64},
        
    };

    m_registerClasses = {
        {{RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8, R9, R10, R11, R12, R13, R14, R15, RIP}, 8, 8},
        {{EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP, R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D}, 4, 4},
        {{AX, BX, CX, DX, SI, DI, BP, SP, R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W}, 2, 2},
        {{AL, BL, CL, DL, SIL, DIL, BPL, SPL, R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B, AH, BH, CH, DH}, 1, 1},
        {{XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15}, 16, 16},
    };

    m_subRegIndexDescs = {
        {"Sub8Bit", 0, 8},
        {"Sub8BitHi", 8, 8},
        {"Sub16Bit", 0, 16},
        {"Sub32Bit", 0, 32},
    };
}

static const uint32_t s_subRegTable[Count][4] = {
    /* RAX   */ { AL,   AH,     AX,        EAX },
    /* RBX   */ { BL,   BH,     BX,        EBX },
    /* RCX   */ { CL,   CH,     CX,        ECX },
    /* RDX   */ { DL,   DH,     DX,        EDX },
    /* RSI   */ { SIL,  0,      SI,        ESI },
    /* RDI   */ { DIL,  0,      DI,        EDI },
    /* RBP   */ { BPL,  0,      BP,        EBP },
    /* RSP   */ { SPL,  0,      SP,        ESP },
    /* R8    */ { R8B,  0,      R8W,       R8D },
    /* R9    */ { R9B,  0,      R9W,       R9D },
    /* R10   */ { R10B, 0,      R10W,      R10D },
    /* R11   */ { R11B, 0,      R11W,      R11D },
    /* R12   */ { R12B, 0,      R12W,      R12D },
    /* R13   */ { R13B, 0,      R13W,      R13D },
    /* R14   */ { R14B, 0,      R14W,      R14D },
    /* R15   */ { R15B, 0,      R15W,      R15D },

    /* EAX   */ { AL,   AH,     AX,        EAX },
    /* EBX   */ { BL,   BH,     BX,        EBX },
    /* ECX   */ { CL,   CH,     CX,        ECX },
    /* EDX   */ { DL,   DH,     DX,        EDX },
    /* ESI   */ { SIL,  0,      SI,        ESI },
    /* EDI   */ { DIL,  0,      DI,        EDI },
    /* EBP   */ { BPL,  0,      BP,        EBP },
    /* ESP   */ { SPL,  0,      SP,        ESP },
    /* R8D   */ { R8B,  0,      R8W,       R8D },
    /* R9D   */ { R9B,  0,      R9W,       R9D },
    /* R10D  */ { R10B, 0,      R10W,      R10D },
    /* R11D  */ { R11B, 0,      R11W,      R11D },
    /* R12D  */ { R12B, 0,      R12W,      R12D },
    /* R13D  */ { R13B, 0,      R13W,      R13D },
    /* R14D  */ { R14B, 0,      R14W,      R14D },
    /* R15D  */ { R15B, 0,      R15W,      R15D },

    /* AX    */ { AL,   AH,     AX,        EAX },
    /* BX    */ { BL,   BH,     BX,        EBX },
    /* CX    */ { CL,   CH,     CX,        ECX },
    /* DX    */ { DL,   DH,     DX,        EDX },
    /* SI    */ { SIL,  0,      SI,        ESI },
    /* DI    */ { DIL,  0,      DI,        EDI },
    /* BP    */ { BPL,  0,      BP,        EBP },
    /* SP    */ { SPL,  0,      SP,        ESP },
    /* R8W   */ { R8B,  0,      R8W,       R8D },
    /* R9W   */ { R9B,  0,      R9W,       R9D },
    /* R10W  */ { R10B, 0,      R10W,      R10D },
    /* R11W  */ { R11B, 0,      R11W,      R11D },
    /* R12W  */ { R12B, 0,      R12W,      R12D },
    /* R13W  */ { R13B, 0,      R13W,      R13D },
    /* R14W  */ { R14B, 0,      R14W,      R14D },
    /* R15W  */ { R15B, 0,      R15W,      R15D },

    /* AL    */ { AL,   0,      AX,        EAX },
    /* BL    */ { BL,   0,      BX,        EBX },
    /* CL    */ { CL,   0,      CX,        ECX },
    /* DL    */ { DL,   0,      DX,        EDX },
    /* SIL   */ { SIL,  0,      SI,        ESI },
    /* DIL   */ { DIL,  0,      DI,        EDI },
    /* BPL   */ { BPL,  0,      BP,        EBP },
    /* SPL   */ { SPL,  0,      SP,        ESP },
    /* R8B   */ { R8B,  0,      R8W,       R8D },
    /* R9B   */ { R9B,  0,      R9W,       R9D },
    /* R10B  */ { R10B, 0,      R10W,      R10D },
    /* R11B  */ { R11B, 0,      R11W,      R11D },
    /* R12B  */ { R12B, 0,      R12W,      R12D },
    /* R13B  */ { R13B, 0,      R13W,      R13D },
    /* R14B  */ { R14B, 0,      R14W,      R14D },
    /* R15B  */ { R15B, 0,      R15W,      R15D },

    /* AH    */ { AH,   0,      AX,        EAX },
    /* BH    */ { BH,   0,      BX,        EBX },
    /* CH    */ { CH,   0,      CX,        ECX },
    /* DH    */ { DH,   0,      DX,        EDX },
};

static const std::vector<uint32_t> s_callerSavedRegisters = {
    RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
};

static const std::vector<uint32_t> s_calleeSavedRegisters = {
    R12, R13, R14, R15, RBX, RBP
};

static const std::vector<uint32_t> s_reservedRegistersGpr64 = {
    R14, R15
};
static const std::vector<uint32_t> s_reservedRegistersGpr32 = {
    R14D, R15D
};
static const std::vector<uint32_t> s_reservedRegistersGpr16 = {
    R14W, R15W
};
static const std::vector<uint32_t> s_reservedRegistersGpr8 = {
    R14B, R15B
};
static const std::vector<uint32_t> s_reservedRegistersFpr = {
    XMM14, XMM15
};

static const std::vector<uint32_t> s_availableRegistersGpr64 = {
    RAX, RBX, RCX, RDX,
    RSI, RDI,
    R8,  R9,  R10, R11,
    R12, R13
};
static const std::vector<uint32_t> s_availableRegistersGpr32 = {
    EAX, EBX, ECX, EDX,
    ESI, EDI,
    R8D, R9D, R10D, R11D,
    R12D, R13D
};
static const std::vector<uint32_t> s_availableRegistersGpr16 = {
    AX, BX, CX, DX,
    SI, DI,
    R8W, R9W, R10W, R11W,
    R12W, R13W
};
static const std::vector<uint32_t> s_availableRegistersGpr8 = {
    AL, BL, CL, DL,
    SIL, DIL,
    R8B, R9B, R10B, R11B,
    R12B, R13B
};
static const std::vector<uint32_t> s_availableRegistersFpr = {
    XMM0, XMM1, XMM2, XMM3,
    XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11,
    XMM12, XMM13
};

uint32_t x64RegisterInfo::getSubReg(uint32_t reg, uint32_t idx) {
    return s_subRegTable[reg][idx];
}

uint32_t x64RegisterInfo::getClassFromType(Type* type) {
    switch(type->getKind()) {
        case Type::TypeKind::Integer: {
            IntegerType* itype = (IntegerType*)(type);
            return itype->getBits() == 64 ? GPR64 : itype->getBits() == 32 ? GPR32 : itype->getBits() == 16 ? GPR16 : GPR8;
        }
        case Type::TypeKind::Float: {
            return FPR;
        }
        case Type::TypeKind::Pointer:
        case Type::TypeKind::Function:
            return GPR64;
        case Type::TypeKind::Void:
        case Type::TypeKind::Struct:
        case Type::TypeKind::Array:
            break;
    }
    assert(false && "Should be unreachable!");
    return GPR64;
}
    
const std::vector<uint32_t>& x64RegisterInfo::getCallerSavedRegisters() const {
    return s_callerSavedRegisters;
}

const std::vector<uint32_t>& x64RegisterInfo::getCalleeSavedRegisters() const {
    return s_calleeSavedRegisters;
}

const std::vector<uint32_t>& x64RegisterInfo::getReservedRegisters(uint32_t rclass) const {
    switch(rclass) {
        case GPR64: return s_reservedRegistersGpr64;
        case GPR32: return s_reservedRegistersGpr32;
        case GPR16: return s_reservedRegistersGpr16;
        case GPR8:  return s_reservedRegistersGpr8;
        case FPR:   return s_reservedRegistersFpr;
        default: break;
    }
    return s_reservedRegistersGpr64;
}

const std::vector<uint32_t>& x64RegisterInfo::getAvailableRegisters(uint32_t rclass) const {
    switch(rclass) {
        case GPR64: return s_availableRegistersGpr64;
        case GPR32: return s_availableRegistersGpr32;
        case GPR16: return s_availableRegistersGpr16;
        case GPR8:  return s_availableRegistersGpr8;
        case FPR:   return s_availableRegistersFpr;
        default: break;
    }
    return s_availableRegistersGpr64;
}

const std::vector<uint32_t>& x64RegisterInfo::getReservedRegistersCanonical(uint32_t rclass) const {
    switch(rclass) {
        case GPR64:
        case GPR32:
        case GPR16:
        case GPR8:  return s_reservedRegistersGpr64;
        case FPR:   return s_reservedRegistersFpr;
        default: break;
    }
    return s_reservedRegistersGpr64;
}

const std::vector<uint32_t>& x64RegisterInfo::getAvailableRegistersCanonical(uint32_t rclass) const {
    switch(rclass) {
        case GPR64:
        case GPR32:
        case GPR16:
        case GPR8:  return s_availableRegistersGpr64;
        case FPR:   return s_availableRegistersFpr;
        default: break;
    }
    return s_availableRegistersGpr64;
}

bool x64RegisterInfo::doClassesOverlap(uint32_t class1, uint32_t class2) const {
    return class1 == class2 || ((class1 >= GPR64 && class1 <= GPR8) && (class2 >= GPR64 && class2 <= GPR8));
}

bool x64RegisterInfo::isFPR(uint32_t rclass) const {
    return rclass == FPR;
}

}