#include "context.hpp"
#include "data_layout.hpp"
#include "type_alias.hpp"
#include "IR/value.hpp"

#include <cstdint>

namespace scbe::IR {

Constant* Constant::getZeroInitalizer(Type* type, DataLayout* layout, Ref<Context> ctx) {
    switch(type->getKind()) {
        case Type::TypeKind::Integer: return ctx->getConstantInt(cast<IntegerType>(type)->getBits(), 0);
        case Type::TypeKind::Float: return ctx->getConstantFloat(cast<FloatType>(type)->getBits(), 0);
        case Type::TypeKind::Pointer: return ctx->getConstantInt(layout->getPointerSize() * 8, 0);
        case Type::TypeKind::Struct: {
            std::vector<IR::Constant*> values;
            StructType* structType = cast<StructType>(type);
            for(Type* contained : structType->getContainedTypes())
                values.push_back(getZeroInitalizer(contained, layout, ctx));
            return ctx->getConstantStruct(structType, values);
        }
        case Type::TypeKind::Array: {
            std::vector<IR::Constant*> values;
            ArrayType* arrayType = cast<ArrayType>(type);
            for(Type* contained : arrayType->getContainedTypes())
                values.push_back(getZeroInitalizer(contained, layout, ctx));
            return ctx->getConstantArray(arrayType, values);
        }
        case Type::TypeKind::Function:
        case Type::TypeKind::Void:
            break;
    }
    throw std::runtime_error("Unsupported type for zero initializer");
}

ConstantInt* ConstantInt::get(uint8_t bits, int64_t value, Ref<Context> context) {
    return context->getConstantInt(bits, value);
}

ConstantFloat* ConstantFloat::get(uint8_t bits, double value, Ref<Context> context) {
    return context->getConstantFloat(bits, value);
}

ConstantString* ConstantString::get(const std::string& value, Ref<Context> context) {
    return context->getConstantString(value);
}

ConstantStruct* ConstantStruct::get(StructType* type, const std::vector<Constant*>& values, Ref<Context> context) {
    return context->getConstantStruct(type, values);
}

ConstantArray* ConstantArray::get(ArrayType* type, const std::vector<Constant*>& values, Ref<Context> context) {
    return context->getConstantArray(type, values);
}

UndefValue* UndefValue::get(Type* type, Ref<Context> context) {
    return context->getUndefValue(type);
}

NullValue* NullValue::get(Type* type, Ref<Context> context) {
    return context->getNullValue(type);
}

}