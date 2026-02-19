#pragma once

#include "value.hpp"

namespace scbe {
class Unit;
}

namespace scbe::MIR {
class GlobalAddress;
}

namespace scbe::IR {

// TODO properly handle this
enum class Linkage {
    External,
    Internal
};

class GlobalValue : public Constant {
public:
    GlobalValue(Type* type, Linkage linkage, ValueKind kind, const std::string& name) : Constant(type, kind, name), m_linkage(linkage) {}

    Linkage getLinkage() const { return m_linkage; }
    void setLinkage(Linkage linkage) { m_linkage = linkage; }

    MIR::GlobalAddress* getMachineGlobalAddress(Unit& unit) ;

protected:
    Linkage m_linkage = Linkage::External;
    MIR::GlobalAddress* m_machineGlobalAddress = nullptr;
};

class GlobalVariable : public GlobalValue {
public:
    Constant* getValue() const { return m_value; }

protected:
    GlobalVariable(Type* type, Constant* value, Linkage linkage, const std::string& name) : GlobalValue(type, linkage, ValueKind::GlobalVariable, name), m_value(value) {}

protected:
    Constant* m_value = nullptr;

friend class scbe::Unit;
};

}