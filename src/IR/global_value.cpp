#include "IR/function.hpp"
#include "MIR/operand.hpp"
#include "unit.hpp"
#include "IR/global_value.hpp"

namespace scbe::IR {

MIR::GlobalAddress* GlobalValue::getMachineGlobalAddress(Unit& unit) {
    if(!m_machineGlobalAddress) {
        auto ptr = std::unique_ptr<MIR::GlobalAddress>(new MIR::GlobalAddress(this));
        m_machineGlobalAddress = ptr.get();
        unit.addSymbol(std::move(ptr));
    }
    return m_machineGlobalAddress;
}

}