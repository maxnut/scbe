#pragma once

#include "type_alias.hpp"
#include "type.hpp"
#include "MIR/operand.hpp"

#include <vector>

namespace scbe::Codegen {
class ISelPass;
}

namespace scbe::IR {
class Function;
class GlobalVariable;
class GlobalValue;
class Constant;
};

namespace scbe::MIR {
class ExternalSymbol;
};

namespace scbe {

class Context;
class PassManager;
class DataLayout;

class Unit {
public:
    Unit(const std::string& name, Ref<Context> ctx) : m_name(name), m_ctx(ctx) {}
    ~Unit();

    IR::Function* addFunction(std::unique_ptr<IR::Function> function);
    IR::GlobalVariable* addGlobal(std::unique_ptr<IR::GlobalVariable> global);
    void print(std::ostream& os);
    void setDataLayout(DataLayout* dataLayout) { m_dataLayout = dataLayout; }
    void addSymbol(std::unique_ptr<MIR::Symbol> symbol) { m_symbols.push_back(std::move(symbol)); }

    const std::string& getName() const { return m_name; }
    const std::vector<std::unique_ptr<IR::Function>>& getFunctions() const { return m_functions; }
    const std::vector<std::unique_ptr<IR::GlobalVariable>>& getGlobals() const { return m_globals; }
    const UMap<std::string, MIR::ExternalSymbol*>& getExternals() const { return m_externals; }
    const UMap<size_t, MIR::GlobalAddress*>& getGlobalAddresses() const { return m_globalAddresses; }
    DataLayout* getDataLayout() const { return m_dataLayout; }

    MIR::ExternalSymbol* getOrInsertExternal(const std::string& name, MIR::ExternalSymbol::Type type);
    MIR::GlobalAddress* getOrInsertGlobalAddress(IR::GlobalValue* value, int64_t flags = 0);
    Ref<Context> getContext() const { return m_ctx; }

    IR::GlobalVariable* createGlobalString(const std::string& value, const std::string& name = "");
    IR::GlobalVariable* createGlobalVariable(Type* type, IR::Constant* value, const std::string& name = "");

    size_t getIRInstructionSize() const;

private:
    std::string m_name;
    std::vector<std::unique_ptr<IR::Function>> m_functions;
    std::vector<std::unique_ptr<IR::GlobalVariable>> m_globals;
    std::vector<std::unique_ptr<MIR::Operand>> m_symbols;
    UMap<std::string, MIR::ExternalSymbol*> m_externals;
    UMap<size_t, MIR::GlobalAddress*> m_globalAddresses;
    Ref<Context> m_ctx;
    DataLayout* m_dataLayout = nullptr;

    UMap<std::string, size_t> m_blockNameStack;

friend class PassManager;
friend class Codegen::ISelPass;
friend class IR::Function;
};

}