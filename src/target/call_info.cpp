#include "target/call_info.hpp"
#include "MIR/function.hpp"
#include "IR/function.hpp"
#include "target/target_specification.hpp"

namespace scbe::Target {

void CallInfo::analyzeCallOperands(CallConvFunction function, MIR::CallLowering* call) {
    m_argAssigns.clear();
    m_argAssigns.resize(call->getTypes().size() - 1);
    function(*this, call->getTypes(), call->isVarArg());
}

void CallInfo::analyzeFormalArgs(CallConvFunction function, MIR::Function* call) {
    m_argAssigns.clear();
    m_argAssigns.resize(call->getIRFunction()->getFunctionType()->getContainedTypes().size() - 1);
    function(*this, call->getIRFunction()->getFunctionType()->getContainedTypes(), call->getIRFunction()->getFunctionType()->isVarArg());
}

CallingConvention getDefaultCallingConvention(TargetSpecification ts) {
    if(ts.getArch() == Arch::x86_64) {
        if(ts.getOS() == OS::Windows) return CallingConvention::Win64;
        return CallingConvention::x64SysV;
    }
    else if(ts.getArch() == Arch::AArch64) return CallingConvention::AAPCS64;
    return CallingConvention::Count;
}

CallConvFunction getCCFunction(CallingConvention cc) {
    switch(cc) {;
        case CallingConvention::Win64: return CCx64Win64;
        case CallingConvention::x64SysV: return CCx64SysV;
        case CallingConvention::AAPCS64: return CCAArch64AAPCS64;
        default: break;
    }
    return nullptr;
}

}