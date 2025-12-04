#pragma once

#include "type.hpp"

#include <algorithm>
#include <string>

namespace scbe {
class Context;
class DataLayout;
}

namespace scbe::IR {

class Instruction;

class Value {
public:
    enum class ValueKind {
        ConstantInt,
        ConstantFloat,
        ConstantString,
        ConstantStruct,
        ConstantArray,
        Block,
        Function,
        GlobalVariable,
        UndefValue,
        NullValue,
        ConstantGEP,
        Register,
        FunctionArgument,
    };

    enum class Flag {
        ByVal = 1 << 0,
        SRet = 1 << 1,
        Count = 1
    };

    virtual ~Value() {}

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    virtual Type* getType() const { return m_type; }
    ValueKind getKind() const { return m_kind; }

    void addUse(Instruction* use) { m_uses.push_back(use); }
    const std::vector<Instruction*>& getUses() const { return m_uses; }
    void removeFromUses(Instruction* use) { m_uses.erase(std::remove(m_uses.begin(), m_uses.end(), use), m_uses.end()); }

    bool hasFlag(Flag flag) const { return (m_flags & (int64_t)flag) != 0; }
    void addFlag(Flag flag) { m_flags |= (int64_t)flag; }
    void setFlags(int64_t flags) { m_flags = flags; }
    void clearFlags() { m_flags = 0; }
    int64_t getFlags() const { return m_flags; }

    bool isConstantInt() const { return m_kind == ValueKind::ConstantInt; }
    bool isConstantFloat() const { return m_kind == ValueKind::ConstantFloat; }
    bool isConstantString() const { return m_kind == ValueKind::ConstantString; }
    bool isConstantStruct() const { return m_kind == ValueKind::ConstantStruct; }
    bool isConstantArray() const { return m_kind == ValueKind::ConstantArray; }
    bool isBlock() const { return m_kind == ValueKind::Block; }
    bool isFunction() const { return m_kind == ValueKind::Function; }
    bool isRegister() const { return m_kind == ValueKind::Register; }
    bool isFunctionArgument() const { return m_kind == ValueKind::FunctionArgument; }
    bool isGlobalVariable() const { return m_kind == ValueKind::GlobalVariable; }
    bool isUndefValue() const { return m_kind == ValueKind::UndefValue; }
    bool isNullValue() const { return m_kind == ValueKind::NullValue; }
    bool isConstantGEP() const { return m_kind == ValueKind::ConstantGEP; }

    bool isConstant() const { return m_kind >= ValueKind::ConstantInt && m_kind <= ValueKind::ConstantArray; }

protected:
    Value(std::string name, Type* type, ValueKind kind) : m_name(std::move(name)), m_type(type), m_kind(kind) {}
    Value() = default;

protected:
    std::string m_name = "";
    Type* m_type = nullptr;
    ValueKind m_kind;
    std::vector<Instruction*> m_uses;
    int64_t m_flags = 0;

friend class Function;
friend class FunctionInlining;
};

class Constant : public Value {
public:
    static Constant* getZeroInitalizer(Type* type, DataLayout* layout, Ref<Context> context);

protected:
    Constant(Type* type, ValueKind kind, const std::string& name = "") : Value(name, type, kind) {}

friend class scbe::Context;
};

class ConstantInt : public Constant {
public:
    static ConstantInt* get(uint8_t bits, int64_t value, Ref<Context> context);

    int64_t getValue() const { return m_value; }

protected:
    ConstantInt(IntegerType* type, int64_t value) : Constant(type, ValueKind::ConstantInt), m_value(value) {}

private:
    int64_t m_value = 0;

friend class scbe::Context;
};

class ConstantFloat : public Constant {
public:
    static ConstantFloat* get(uint8_t bits, double value, Ref<Context> context);

    double getValue() const { return m_value; }
    void setValue(double value) { m_value = value; }

protected:
    ConstantFloat(FloatType* type, double value) : Constant(type, ValueKind::ConstantFloat), m_value(value) {}

private:
    double m_value = 0.0;

friend class scbe::Context;
};

class ConstantString : public Constant {
public:
    static ConstantString* get(const std::string& value, Ref<Context> context);

    const std::string& getValue() const { return m_value; }
    void setValue(const std::string& value) { m_value = value; }

protected:
    ConstantString(Type* type, const std::string& value) : Constant(type, ValueKind::ConstantString), m_value(value) {}

private:
    std::string m_value = "";

friend class scbe::Context;
};

class ConstantMultiple : public Constant {
public:
    const std::vector<Constant*>& getValues() const { return m_values; }

protected:
    ConstantMultiple(ValueKind kind, Type* type, const std::vector<Constant*>& values) : Constant(type, kind), m_values(values) {}
    
private:
    std::vector<Constant*> m_values;
};

class ConstantStruct : public ConstantMultiple {
public:
    static ConstantStruct* get(StructType* type, const std::vector<Constant*>& values, Ref<Context> context);

protected:
    ConstantStruct(StructType* type, const std::vector<Constant*>& values) : ConstantMultiple(ValueKind::ConstantStruct, type, values) {}

friend class scbe::Context;
};

class ConstantArray : public ConstantMultiple {
public:
    static ConstantArray* get(ArrayType* type, const std::vector<Constant*>& values, Ref<Context> context);

protected:
    ConstantArray(ArrayType* type, const std::vector<Constant*>& values) : ConstantMultiple(ValueKind::ConstantArray, type, values) {}

friend class scbe::Context;
};

class FunctionArgument : public Value {
public:
    FunctionArgument(const std::string& name, Type* type, uint32_t slot) : Value(name, type, ValueKind::FunctionArgument), m_slot(slot) {}

    uint32_t getSlot() const { return m_slot; }

private:
    uint32_t m_slot = 0;
};

class UndefValue : public Constant {
public:
    static UndefValue* get(Type* type, Ref<Context> context);

protected:
    UndefValue(Type* type) : Constant(type, ValueKind::UndefValue) {}

friend class scbe::Context;
};

class NullValue : public Constant {
public:
    static NullValue* get(Type* type, Ref<Context> context);

protected:
    NullValue(Type* type) : Constant(type, ValueKind::NullValue) {}

friend class scbe::Context;
};

class ConstantGEP : public Constant {
public:
    static ConstantGEP* get(Constant* base, const std::vector<Constant*>& indices, Ref<Context> ctx);

    Constant* getBase() const { return m_base; }
    const std::vector<Constant*>& getIndices() const { return m_indices; }

    size_t calculateOffset(DataLayout* layout);

private:
    ConstantGEP(Constant* base, const std::vector<Constant*>& indices, Type* resultTy)
        : Constant(resultTy, ValueKind::ConstantGEP), m_base(base), m_indices(indices) {}

    Constant* m_base;
    std::vector<Constant*> m_indices;

friend class scbe::Context;
};

}