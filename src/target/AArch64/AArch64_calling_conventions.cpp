#include "target/AArch64/AArch64_register_info.hpp"
#include "target/call_info.hpp"

#include <array>

using namespace scbe::Target::AArch64;

namespace scbe::Target {

struct ArgAssignmentState {
    size_t m_usedGPR = 0;
    size_t m_usedFPR = 0;

    static constexpr std::array<RegisterId, 8> GPRs = {X0, X1, X2, X3, X4, X5, X6, X7};
    static constexpr std::array<RegisterId, 8> FPRs = {D0, D1, D2, D3, D4, D5, D6, D7};

    bool hasGPR() const { return m_usedGPR < GPRs.size(); }
    bool hasFPR() const { return m_usedFPR < FPRs.size(); }

    RegisterId nextGPR() { return GPRs[m_usedGPR++]; }
    RegisterId nextFPR() { return FPRs[m_usedFPR++]; }
};

void CCAArch64AAPCS64(CallInfo& info, const std::vector<Type*>& types, bool isVarArg) {
    ArgAssignmentState state;

    for (size_t i = 0; i < types.size() - 1; ++i) {
        Type* type = types[i + 1];

        bool isFloat = type->isFltType();
        size_t size = info.getDataLayout()->getSize(type);
        size_t align = info.getDataLayout()->getAlignment(type);

        if ((isFloat && state.hasFPR()) || (!isFloat && state.hasGPR())) {
            RegisterId baseReg = isFloat ? state.nextFPR() : state.nextGPR();
            size_t registerSize = info.getRegisterInfo()->getRegisterClass(
                info.getRegisterInfo()->getRegisterDesc(baseReg).getRegClass()
            ).getSize();
            uint32_t rId = size == registerSize ? baseReg : info.getRegisterInfo()->getRegisterWithSize(baseReg, size).value();

            Ref<ArgAssign> assign = std::make_shared<RegisterAssign>(rId, size);
            info.setArgAssign(i, assign);
        } else {
            Ref<ArgAssign> assign = std::make_shared<StackAssign>();
            info.setArgAssign(i, assign);
        }
    }

    // Return value
    Type* retType = types[0];
    if (retType->isVoidType()) {
        // info.addRetAssign(nullptr);
        return;
    }
    else if (retType->isStructType()) {
        size_t size = info.getDataLayout()->getSize(retType);
        if (size <= 16) {
            info.addRetAssign(std::make_shared<RegisterAssign>(X0, 8));
            if (size > 8)
                info.addRetAssign(std::make_shared<RegisterAssign>(X1, 8));
        }
        return;
    }

    RegisterId retReg = retType->isFltType() ? D0 : X0;
    size_t size = info.getDataLayout()->getSize(retType);
    uint32_t retId = size == 8 ? retReg : info.getRegisterInfo()->getRegisterWithSize(retReg, size).value();

    info.addRetAssign(std::make_shared<RegisterAssign>(retId, size));
}


}
