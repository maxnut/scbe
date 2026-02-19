#pragma once

#include "calling_convention.hpp"
#include "node.hpp"
#include "type.hpp"
#include "value.hpp"

namespace scbe::ISel {

class Instruction : public Node {
public:
    Instruction(NodeKind kind) : Node(kind) {}
    Instruction(NodeKind kind, Value* result) : Node(kind), m_result(result) {}
    Instruction(NodeKind kind, Value* result, Node* operand) : Node(kind), m_result(result) { m_operands.push_back(operand); }
    Instruction(NodeKind kind, Value* result, Node* lhs, Node* rhs) : Node(kind), m_result(result) { m_operands.push_back(lhs); m_operands.push_back(rhs); }

    const std::vector<Node*>& getOperands() const { return m_operands; }
    Value* getResult() const { return m_result; }

    void addOperand(Node* operand) { m_operands.push_back(operand); }

protected:
    std::vector<Node*> m_operands;
    Value* m_result = nullptr;
};

class Root : public Node {
public:
    Root(const std::string& name) : Node(NodeKind::Root), m_name(name) {}

    const std::string& getName() const { return m_name; }

public:
    std::vector<std::unique_ptr<Node>> m_nodes; // root is the owner of all nodes
    std::vector<Instruction*> m_instructions;

private:
    std::string m_name;
};

class Call : public Instruction {
public:
    // Call(Value* result, Node* callee, bool isResultUsed) : Chain(NodeKind::Call, result), m_isResultUsed(isResultUsed) { addOperand(callee); }
    Call(Value* result, CallingConvention cc) : Instruction(NodeKind::Call, result) {}

    bool isResultUsed() const { return m_isResultUsed; }
    void setResultUsed(bool isResultUsed) { m_isResultUsed = isResultUsed; }
    CallingConvention getCallingConvention() const { return m_callConv; }

private:
    bool m_isResultUsed = true;
    CallingConvention m_callConv = CallingConvention::Count;
};

class Cast : public Instruction {
public:
    Cast(NodeKind kind, Register* result, Node* value, Type* type) : Instruction(kind, result), m_type(type) { addOperand(value); }
    Cast(NodeKind kind, Register* result, Type* type) : Instruction(kind, result), m_type(type) {}

    Type* getType() const { return m_type; }

private:
    Type* m_type;
};

class DynamicAllocation : public Instruction {
public:
    DynamicAllocation(Register* result, Type* type) : Instruction(Node::NodeKind::DynamicAllocation, result), m_type(type) {}

    Type* getType() const { return m_type; }

private:
    Type* m_type;
};

}