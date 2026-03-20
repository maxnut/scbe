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
        case Type::TypeKind::Pointer: return ctx->getNullValue(type);
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
            Constant* zero = getZeroInitalizer(arrayType->getElement(), layout, ctx);
            for(size_t i = 0; i < arrayType->getScale(); i++)
                values.push_back(zero);
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

ConstantStruct* ConstantStruct::get(StructType* type, const std::vector<Constant*>& values, Ref<Context> context) {
    return context->getConstantStruct(type, values);
}

ConstantArray* ConstantArray::get(ArrayType* type, const std::vector<Constant*>& values, Ref<Context> context) {
    return context->getConstantArray(type, values);
}

ConstantArray* ConstantArray::fromString(const std::string& str, Ref<Context> context) {
    ArrayType* type = context->makeArrayType(context->getI8Type(), str.size() + 1);
    std::vector<Constant*> chars;
    chars.reserve(str.size());
    for(auto& c : str) chars.push_back(context->getConstantInt(8, c));
    chars.push_back(context->getConstantInt(8, '\0'));
    return context->getConstantArray(type, chars);
}

UndefValue* UndefValue::get(Type* type, Ref<Context> context) {
    return context->getUndefValue(type);
}

NullValue* NullValue::get(Type* type, Ref<Context> context) {
    return context->getNullValue(type);
}

ConstantGEP* ConstantGEP::get(Constant* base, const std::vector<ConstantInt*>& indices, Ref<Context> ctx) {
    return ctx->getConstantGEP(base, indices);
}

size_t ConstantGEP::calculateOffset(DataLayout* layout) {
    size_t off = 0;
    if(auto dynagep = dyn_cast<ConstantGEP>(m_base)) off += dynagep->calculateOffset(layout);
    Type* curType = m_base->getType();
    for(auto index : m_indices) {
        switch (index->getKind()) {
            case IR::Value::ValueKind::ConstantInt: {
                IR::ConstantInt* constant = (IR::ConstantInt*)index;
                for(size_t i = 0; i < constant->getValue(); i++) {
                    Type* ty = curType->isStructType() ? curType->getContainedTypes().at(i) : curType->getContainedTypes().at(0);
                    off += layout->getSize(ty);
                }
                curType = curType->isStructType() ? curType->getContainedTypes().at(constant->getValue()) : curType->getContainedTypes().at(0);
                break;
            }
            default: throw std::runtime_error("Unsupported index type");
        }
    }
    return off;
}

class ConstantPointer : public ConstantMultiple {
public:
    ConstantPointer(std::vector<Constant*> pointees, PointerType* ty) : ConstantMultiple(ValueKind::ConstantPointer, ty, pointees) {}
};

std::vector<std::unique_ptr<ConstantPointer>> g_cptr;

ConstantPointer* tryEvaluateInternal(IR::ConstantGEP* gep, Ref<Context> ctx) {
    IR::Constant* current = gep->getBase();
    if(auto dynagep = dyn_cast<ConstantGEP>(current)) current = tryEvaluateInternal(dynagep, ctx);

    std::vector<Constant*> pointees;
    for(size_t i = 0; i < gep->getIndices().size(); i++) {
        auto& idx = gep->getIndices().at(i);
        switch(current->getKind()) {
            case IR::Value::ValueKind::ConstantArray:
            case IR::Value::ValueKind::ConstantStruct:
            case IR::Value::ValueKind::ConstantPointer: {
                IR::ConstantMultiple* multi = cast<ConstantMultiple>(current);
                if(i == gep->getIndices().size() - 1) {
                    for(size_t j = idx->getValue(); j < multi->getValues().size(); j++) {
                        pointees.push_back(multi->getValues().at(j));
                    }
                    break;
                }
                current = multi->getValues().at(idx->getValue());
                break;
            }
            case IR::Value::ValueKind::GlobalVariable: {
                if(idx->getValue() != 0) return nullptr;
                IR::GlobalVariable* ptr = cast<GlobalVariable>(current);
                if(i == gep->getIndices().size() - 1) {
                    pointees.push_back(ptr->getValue());
                    break;
                }
                current = ptr->getValue();
                break;
            }
            default: return nullptr;
        }
    }
    std::unique_ptr<ConstantPointer> tmp = std::make_unique<ConstantPointer>(pointees, ctx->makePointerType(current->getType()));
    ConstantPointer* ret = tmp.get();
    g_cptr.push_back(std::move(tmp));
    return ret;
}

Constant* ConstantGEP::tryEvaluate(Ref<Context> ctx) {
    auto res = tryEvaluateInternal(this, ctx);
    auto ret = res ? res->getValues().front() : nullptr;
    g_cptr.clear();
    return ret;
}

}