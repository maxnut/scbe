#include "context.hpp"
#include "IR/value.hpp"
#include "target/AArch64/AArch64_target.hpp"
#include "target/target_specification.hpp"
#include "target/x64/x64_target.hpp"
#include "type.hpp"
#include "hash.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

namespace scbe {

Context::Context() {
    m_types.reserve(8);
    m_types.push_back(std::unique_ptr<Type>(new IntegerType(1)));
    m_types.push_back(std::unique_ptr<Type>(new IntegerType(8)));
    m_types.push_back(std::unique_ptr<Type>(new IntegerType(16)));
    m_types.push_back(std::unique_ptr<Type>(new IntegerType(32)));
    m_types.push_back(std::unique_ptr<Type>(new IntegerType(64)));
    m_types.push_back(std::unique_ptr<Type>(new FloatType(32)));
    m_types.push_back(std::unique_ptr<Type>(new FloatType(64)));
    m_types.push_back(std::unique_ptr<Type>(new VoidType()));
}

Context::~Context() = default;

IntegerType *Context::getIntegerType(uint8_t bits) const {
  switch (bits) {
  case 1:
    return getI1Type();
  case 8:
    return getI8Type();
  case 16:
    return getI16Type();
  case 32:
    return getI32Type();
  case 64:
    return getI64Type();
  default:
    break;
  }
  throw std::runtime_error("Unsupported integer type");
}

FloatType* Context::getFloatType(uint8_t bits) const {
    switch(bits) {
        case 32: return getF32Type();
        case 64: return getF64Type();
        default: break;
    }
    throw std::runtime_error("Unsupported float type");
}

IntegerType* Context::getI1Type() const {
    return (IntegerType*)(m_types[0].get());
}

IntegerType* Context::getI8Type() const {
    return (IntegerType*)(m_types[1].get());
}

IntegerType* Context::getI16Type() const {
    return (IntegerType*)(m_types[2].get());
}

IntegerType* Context::getI32Type() const {
    return (IntegerType*)(m_types[3].get());
}

IntegerType* Context::getI64Type() const {
    return (IntegerType*)(m_types[4].get());
}

FloatType* Context::getF32Type() const {
    return (FloatType*)(m_types[5].get());
}

FloatType* Context::getF64Type() const {
    return (FloatType*)(m_types[6].get());
}

VoidType* Context::getVoidType() const {
    return (VoidType*)(m_types[7].get());
}

PointerType* Context::makePointerType(Type* base) {
    size_t hash = std::hash<Type*>{}(base);
    if(auto it = m_pointerCache.find(hash); it != m_pointerCache.end())
        return it->second;

    auto type = std::unique_ptr<PointerType>(new PointerType(base));
    PointerType* ret = type.get();
    m_pointerCache[hash] = ret;
    m_types.push_back(std::move(type));
    return ret;
}

ArrayType* Context::makeArrayType(Type* base, uint32_t size) {
    // size_t hash = std::hash<Type*>{}(base);
    // hash ^= size + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    size_t hash = hashValues((size_t)base, size);
    if(auto it = m_arrayCache.find(hash); it != m_arrayCache.end())
        return it->second;

    auto type = std::unique_ptr<ArrayType>(new ArrayType(base, size));
    ArrayType* ret = type.get();
    m_arrayCache[hash] = ret;
    m_types.push_back(std::move(type));
    return ret;
}

StructType* Context::makeStructType(std::vector<Type*> elements, const std::string& name) {
    if(elements.size() == 0) {
        auto type = std::unique_ptr<StructType>(new StructType(elements, name));
        StructType* ret = type.get();
        m_types.push_back(std::move(type));
        return ret;
    }
    size_t hash = hashTypes(elements);
    hash ^= std::hash<std::string>{}(name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    if(auto it = m_structCache.find(hash); it != m_structCache.end())
        return it->second;

    auto type = std::unique_ptr<StructType>(new StructType(elements, name));
    StructType* ret = type.get();
    m_structCache[hash] = ret;
    m_types.push_back(std::move(type));
    return ret;
}

void Context::updateStructType(StructType* type, std::vector<Type*> elements) {
    size_t hash = hashTypes(type->getContainedTypes());
    hash ^= std::hash<std::string>{}(type->m_name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    if(m_structCache.contains(hash))
        m_structCache.erase(hash);
    m_structCache[hash] = type;
    type->m_containedTypes = std::move(elements);
}

FunctionType* Context::makeFunctionType(std::vector<Type*> parameters, Type* returnType) {
    size_t hash = hashTypes(parameters);
    hash ^= std::hash<Type*>{}(returnType) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    if(auto it = m_functionCache.find(hash); it != m_functionCache.end())
        return it->second;

    auto type = std::unique_ptr<FunctionType>(new FunctionType(parameters, returnType));
    FunctionType* ret = type.get();
    m_functionCache[hash] = ret;
    m_types.push_back(std::move(type));
    return ret;
}


void Context::registerAllTargets() {
    m_targetRegistry.registerTarget(Target::Arch::x86_64, std::make_shared<Target::x64::x64Target>());
    m_targetRegistry.registerTarget(Target::Arch::AArch64, std::make_shared<Target::AArch64::AArch64Target>());
}

IR::ConstantInt* Context::getConstantInt(uint8_t bits, int64_t value) {
    size_t hash = hashValues(bits, value);
    if(m_constantIntCache.contains(hash)) return m_constantIntCache.at(hash);
    std::unique_ptr<IR::ConstantInt> constant(new IR::ConstantInt(getIntegerType(bits), value));
    IR::ConstantInt* ret = constant.get();
    m_constantIntCache.insert({hash, ret});
    m_constants.push_back(std::move(constant));
    return ret;
}

IR::ConstantFloat* Context::getConstantFloat(uint8_t bits, double value) {
    size_t hash = hashValues(bits, value);
    if(m_constantFloatCache.contains(hash)) return m_constantFloatCache.at(hash);
    std::unique_ptr<IR::ConstantFloat> constant(new IR::ConstantFloat(getFloatType(bits), value));
    IR::ConstantFloat* ret = constant.get();
    m_constantFloatCache.insert({hash, ret});
    m_constants.push_back(std::move(constant));
    return ret;
}

IR::ConstantString* Context::getConstantString(const std::string& value) {
    if(m_constantStringCache.contains(value)) return m_constantStringCache.at(value);
    std::unique_ptr<IR::ConstantString> constant(new IR::ConstantString(makePointerType(getI8Type()), value));
    IR::ConstantString* ret = constant.get();
    m_constantStringCache.insert({value, ret});
    m_constants.push_back(std::move(constant));
    return ret;
}

IR::ConstantStruct* Context::getConstantStruct(StructType* type, const std::vector<IR::Constant*>& values) {
    for(size_t i = 0; i < values.size(); i++)
        assert(values.at(i)->getType() == type->getContainedTypes().at(i) && "Value type mismatch");

    std::unique_ptr<IR::ConstantStruct> constant(new IR::ConstantStruct(type, values)); // TODO cache
    IR::ConstantStruct* ret = constant.get();
    m_constants.push_back(std::move(constant));
    return ret;
}

IR::ConstantArray* Context::getConstantArray(ArrayType* type, const std::vector<IR::Constant*>& values) {
    for(auto value : values)
        assert(value->getType() == type->getElement() && "Value type mismatch");

    std::unique_ptr<IR::ConstantArray> constant(new IR::ConstantArray(type, values)); // TODO cache
    IR::ConstantArray* ret = constant.get();
    m_constants.push_back(std::move(constant));
    return ret;
}

IR::UndefValue* Context::getUndefValue(Type* type) {
    if(m_undefValueCache.contains(type)) return m_undefValueCache.at(type);
    std::unique_ptr<IR::UndefValue> undef(new IR::UndefValue(type));
    IR::UndefValue* ret = undef.get();
    m_undefValueCache.insert({type, ret});
    m_constants.push_back(std::move(undef));
    return ret;
}

IR::NullValue* Context::getNullValue(Type* type) {
    if(m_nullValueCache.contains(type)) return m_nullValueCache.at(type);
    std::unique_ptr<IR::NullValue> null(new IR::NullValue(type));
    IR::NullValue* ret = null.get();
    m_nullValueCache.insert({type, ret});
    m_constants.push_back(std::move(null));
    return ret;
}

MIR::ImmediateInt* Context::getImmediateInt(int64_t value, MIR::ImmediateInt::Size size, int64_t flags) {
    size_t hash = hashValues(value, size, flags);
    if(m_immediateIntCache.contains(hash)) return m_immediateIntCache.at(hash);
    std::unique_ptr<MIR::ImmediateInt> immediate(new MIR::ImmediateInt(value, size));
    immediate->setFlags(flags);
    MIR::ImmediateInt* ret = immediate.get();
    m_immediates.push_back(std::move(immediate));
    m_immediateIntCache.insert({hash, ret});
    return ret;
}

}