#include "unit.hpp"
#include "IR/global_value.hpp"
#include "IR/printer.hpp"
#include "IR/function.hpp"
#include "MIR/operand.hpp"

namespace scbe{

Unit::~Unit() = default;

IR::Function* Unit::addFunction(std::unique_ptr<IR::Function> function) {
    IR::Function* ret = function.get();
    function->m_unit = this;
    m_functions.push_back(std::move(function));
    return ret;
}

void Unit::print(std::ostream& os) {
    IR::HumanPrinter(os).print(*this);
}

IR::GlobalVariable* Unit::addGlobal(std::unique_ptr<IR::GlobalVariable> global) {
    IR::GlobalVariable* ret = global.get();
    if(global->getName().empty())
        global->setName("global" + std::to_string(m_globals.size()));
    m_globals.push_back(std::move(global));
    return ret;
}

MIR::ExternalSymbol* Unit::getOrInsertExternal(const std::string& name, MIR::ExternalSymbol::Type type) {
    if(!m_externals.contains(name)) {
        auto ptr = std::unique_ptr<MIR::ExternalSymbol>(new MIR::ExternalSymbol(name, type));
        auto ret = ptr.get();
        m_externals[name] = ret;
        m_symbols.push_back(std::move(ptr));
        return ret;
    }
    return m_externals[name];
}

MIR::GlobalAddress* Unit::getOrInsertGlobalAddress(IR::GlobalValue* value, int64_t flags) {
    size_t hash = std::hash<IR::GlobalValue*>{}(value) ^ std::hash<int64_t>{}(flags);
    if(!m_globalAddresses.contains(hash)) {
        auto ptr = value->getMachineGlobalAddress(*this);
        ptr->setFlags(flags);
        m_globalAddresses[hash] = ptr;
        return ptr;
    }
    return m_globalAddresses[hash];
}

IR::GlobalVariable* Unit::createGlobalString(const std::string& value, const std::string& name) {
    IR::ConstantString* constant = IR::ConstantString::get(value, m_ctx);
    return createGlobalVariable(m_ctx->getI8Type(), constant, name);
}

IR::GlobalVariable* Unit::createGlobalVariable(Type* type, IR::Constant* value, const std::string& name) {
    if(!value) getOrInsertExternal(name, MIR::ExternalSymbol::Type::Variable);
    return IR::GlobalVariable::get(*this, m_ctx->makePointerType(type), value, IR::Linkage::External, name);
}

size_t Unit::getIRInstructionSize() const {
    size_t size = 0;
    for(const auto& func : m_functions)
        size += func->getInstructionSize();
    return size;
}

}