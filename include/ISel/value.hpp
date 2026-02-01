#pragma once

#include "type.hpp"
#include "node.hpp"

namespace scbe::ISel {

class Value : public Node {
public:
    Type* getType() const { return m_type; }

protected:
    Value(NodeKind kind, Type* type) : Node(kind), m_type(type) {}

protected:
    Type* m_type;
};

class ConstantInt : public Value {
public:
    ConstantInt(int64_t value, Type* type) : Value(NodeKind::ConstantInt, type), m_value(value) {}

    int64_t getValue() const { return m_value; }

private:
    int64_t m_value;
};

class ConstantFloat : public Value {
public:
    ConstantFloat(double value, Type* type) : Value(NodeKind::ConstantFloat, type), m_value(value) {}

    double getValue() const { return m_value; }

private:
    double m_value;
};

class Register : public Value {
public:
    Register(std::string name, Type* type) : Value(NodeKind::Register, type), m_name(name) {}

    std::string getName() const { return m_name; }

private:
    std::string m_name;
};

class FrameIndex : public Value {
public:
    FrameIndex(uint32_t slot, Type* type) : Value(NodeKind::FrameIndex, type), m_slot(slot) {}

    uint32_t getSlot() const { return m_slot; }

private:
    uint32_t m_slot;
};

class FunctionArgument : public Value {
public:
    FunctionArgument(uint32_t slot, Type* type) : Value(NodeKind::FunctionArgument, type), m_slot(slot) {}

    uint32_t getSlot() const { return m_slot; }

private:
    uint32_t m_slot;
};

class GlobalValue : public Value {
public:
    GlobalValue(IR::GlobalValue* global) : Value(NodeKind::GlobalValue, global->getType()), m_global(global) {}

    IR::GlobalValue* getGlobal() const { return m_global; }

protected:
    IR::GlobalValue* m_global;
};

class MultiValue : public Value {
public:
    MultiValue(Type* type) : Value(NodeKind::MultiValue, type) {}

    void addValue(Value* value) { m_values.push_back(value); }
    const std::vector<Value*>& getValues() const { return m_values; }

private:
    std::vector<Value*> m_values;
};

}