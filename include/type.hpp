#pragma once

#include <span>
#include <string>
#include <vector>

#include "type_alias.hpp"

namespace scbe{
class Context;

class Type {
public:
    enum class TypeKind {
        Integer,
        Float,
        Pointer,
        Void,
        Struct,
        Array,
        Function
    };

    TypeKind getKind() const { return m_kind; }

    bool isIntType() const { return m_kind == TypeKind::Integer; }
    bool isFltType() const { return m_kind == TypeKind::Float; }
    bool isPtrType() const { return m_kind == TypeKind::Pointer; }
    bool isVoidType() const { return m_kind == TypeKind::Void; }
    bool isStructType() const { return m_kind == TypeKind::Struct; }
    bool isArrayType() const { return m_kind == TypeKind::Array; }
    bool isFuncType() const { return m_kind == TypeKind::Function; }

    const std::vector<Type*>& getContainedTypes() const { return m_containedTypes; }

protected:
    Type(TypeKind kind) : m_kind(kind) {}

protected:
    TypeKind m_kind;
    std::vector<Type*> m_containedTypes;

friend class scbe::Context;
};

class VoidType : public Type {
public:

protected:
    VoidType() : Type(Type::TypeKind::Void) {}

friend class scbe::Context;
};

class IntegerType : public Type {
public:
    uint8_t getBits() const { return m_bits; }

protected:
    IntegerType(uint8_t bits) : Type(Type::TypeKind::Integer), m_bits(bits) {}

protected:
    uint8_t m_bits;    

friend class scbe::Context;
};

class FloatType : public Type {
public:
    uint8_t getBits() const { return m_bits; }

protected:
    FloatType(uint8_t bits) : Type(Type::TypeKind::Float), m_bits(bits) {}

protected:
    uint8_t m_bits;    

friend class scbe::Context;
};

class PointerType : public Type {
public:
    Type* getPointee() const { return m_containedTypes.front(); }

protected:
    PointerType(Type* pointee) : Type(Type::TypeKind::Pointer) { m_containedTypes.push_back(pointee); }

friend class scbe::Context;
};

class StructType : public Type {
public:
    const std::string& getName() const { return m_name; }

protected:
    StructType(std::vector<Type*> elements, const std::string& name) : Type(Type::TypeKind::Struct), m_name(name) { m_containedTypes = std::move(elements); }

protected:
    std::string m_name;

friend class scbe::Context;
};

class ArrayType : public Type {
public:
    Type* getElement() const { return m_containedTypes.front(); }
    uint32_t getScale() const { return m_scale; }

protected:
    ArrayType(Type* element, uint32_t scale) : Type(Type::TypeKind::Array), m_scale(scale) { m_containedTypes.push_back(element); }

protected:
    uint32_t m_scale;

friend class scbe::Context;
};

class FunctionType : public Type {
public:
    std::span<Type*> getArguments() { return std::span<Type*>(m_containedTypes.data() + 1, m_containedTypes.size() - 1); }
    Type* getReturnType() const { return m_containedTypes.front(); }
    bool isVarArg() const { return m_isVarArg; }

protected:
    FunctionType(std::vector<Type*>& parameters, Type* returnType, bool isVarArg) : Type(Type::TypeKind::Function), m_isVarArg(isVarArg) {
        m_containedTypes.push_back(returnType);
        m_containedTypes.insert(m_containedTypes.end(), parameters.begin(), parameters.end());
    }

protected:
    bool m_isVarArg = false;

friend class scbe::Context;
};

}