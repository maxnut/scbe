#include "unit.hpp"
#include "hash.hpp"
#include "IR/global_value.hpp"
#include "IR/printer.hpp"
#include "IR/function.hpp"
#include "MIR/operand.hpp"

namespace scbe{

Unit::~Unit() = default;

IR::Function* Unit::getOrInsertFunction(std::string name, FunctionType* type, IR::Linkage linkage) {
    if(name.empty()) return nullptr;
    if(m_symbolTable.contains(name)) return cast<IR::Function>(m_symbolTable.at(name));

    auto function = std::make_unique<IR::Function>(name, type, linkage);
    IR::Function* ret = function.get();
    function->m_unit = this;
    m_functions.push_back(std::move(function));
    return ret;
}

IR::Function* Unit::getOrInsertFunction(IR::IntrinsicFunction::Name name) {
    auto in = IR::IntrinsicFunction::get(name, m_ctx);
    if(m_symbolTable.contains(in->getName())) return cast<IR::Function>(m_symbolTable.at(in->getName()));
    in->m_unit = this;
    IR::Function* ret = in.get();
    m_functions.push_back(std::move(in));
    return ret;
}

void Unit::print(std::ostream& os) {
    IR::HumanPrinter(os).print(*this);
}

IR::GlobalVariable* Unit::getOrInsertGlobalVariable(Type* type, IR::Constant* value, IR::Linkage linkage, std::string name) {
    if(name.empty()) name = "global" + std::to_string(m_globals.size());
    if(m_symbolTable.contains(name)) return cast<IR::GlobalVariable>(m_symbolTable.at(name));

    std::unique_ptr<IR::GlobalVariable> global = std::unique_ptr<IR::GlobalVariable>(new IR::GlobalVariable(type, value, linkage, name));
    IR::GlobalVariable* ret = global.get();
    m_globals.push_back(std::move(global));
    if(!value) getOrInsertExternal(name, MIR::ExternalSymbol::Type::Variable);
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
    size_t hash = hashValues(value, flags);
    if(!m_globalAddresses.contains(hash)) {
        auto ptr = value->getMachineGlobalAddress(*this);
        ptr->setFlags(flags);
        m_globalAddresses[hash] = ptr;
        return ptr;
    }
    return m_globalAddresses[hash];
}

IR::GlobalVariable* Unit::createGlobalString(const std::string& value) {
    IR::ConstantString* constant = IR::ConstantString::get(value, m_ctx);
    return getOrInsertGlobalVariable(constant->getType(), constant, IR::Linkage::External);
}

size_t Unit::getIRInstructionSize() const {
    size_t size = 0;
    for(const auto& func : m_functions)
        size += func->getInstructionSize();
    return size;
}

}