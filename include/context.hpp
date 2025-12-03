#pragma once

#include "target/target_registry.hpp"
#include "type.hpp"
#include "IR/value.hpp"
#include "MIR/operand.hpp"

#include <vector>

namespace scbe {
namespace IR {
class Module;
}

class Context {
public:
    Context();
    ~Context();

    IntegerType* getIntegerType(uint8_t bits) const;
    IntegerType* getI1Type() const;
    IntegerType* getI8Type() const;
    IntegerType* getI16Type() const;
    IntegerType* getI32Type() const;
    IntegerType* getI64Type() const;
    FloatType* getF32Type() const;
    FloatType* getF64Type() const;
    FloatType* getFloatType(uint8_t bits) const;
    VoidType* getVoidType() const;

    StructType* makeStructType(std::vector<Type*> elements, const std::string& name);
    void updateStructType(StructType* type, std::vector<Type*> elements);
    ArrayType* makeArrayType(Type* base, uint32_t size);
    PointerType* makePointerType(Type* base);
    FunctionType* makeFunctionType(std::vector<Type*> parameters, Type* returnType);

    void registerAllTargets();

    const Target::TargetRegistry& getTargetRegistry() const { return m_targetRegistry; }

    IR::ConstantInt* getConstantInt(uint8_t bits, int64_t value);
    IR::ConstantFloat* getConstantFloat(uint8_t bits, double value);
    IR::ConstantString* getConstantString(const std::string& value);
    IR::ConstantStruct* getConstantStruct(StructType* type, const std::vector<IR::Constant*>& values);
    IR::ConstantArray* getConstantArray(ArrayType* type, const std::vector<IR::Constant*>& values);
    IR::ConstantGEP* getConstantGEP(IR::Constant* base, const std::vector<IR::Constant*>& indices);
    IR::UndefValue* getUndefValue(Type* type);    
    IR::NullValue* getNullValue(Type* type);    

    MIR::ImmediateInt* getImmediateInt(int64_t value, MIR::ImmediateInt::Size size, int64_t flags = 0);
    
private:
    std::vector<std::unique_ptr<Type>> m_types;
    std::vector<std::unique_ptr<IR::Constant>> m_constants;
    std::vector<std::unique_ptr<MIR::ImmediateInt>> m_immediates;
    Target::TargetRegistry m_targetRegistry;

    UMap<size_t, StructType*> m_structCache;
    UMap<size_t, ArrayType*> m_arrayCache;
    UMap<size_t, PointerType*> m_pointerCache;
    UMap<size_t, FunctionType*> m_functionCache;

    UMap<size_t, IR::ConstantInt*> m_constantIntCache;
    UMap<size_t, IR::ConstantFloat*> m_constantFloatCache;
    UMap<std::string, IR::ConstantString*> m_constantStringCache;
    UMap<Type*, IR::UndefValue*> m_undefValueCache;
    UMap<Type*, IR::NullValue*> m_nullValueCache;

    UMap<size_t, MIR::ImmediateInt*> m_immediateIntCache;

    friend class IR::Module;
};

}