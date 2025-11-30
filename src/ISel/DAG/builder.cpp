#include "ISel/DAG/builder.hpp"
#include "type.hpp"
#include "ISel/DAG/instruction.hpp"
#include "ISel/DAG/node.hpp"
#include "ISel/DAG/value.hpp"
#include <memory>

namespace scbe::ISel::DAG {

Builder::~Builder() = default;

void Builder::insert(std::unique_ptr<Node> node) {
    assert(m_root && "Root is not set");
    node->setRoot(m_root);
    m_root->m_nodes.push_back(std::move(node));
}

std::unique_ptr<Root> Builder::createRoot(const std::string& name) {
    return std::move(std::make_unique<Root>(name));
}

Node* Builder::createAdd(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::Add, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createSub(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::Sub, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createICmpEq(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ICmpEq, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createICmpNe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ICmpNe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createICmpGt(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ICmpGt, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createICmpGe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ICmpGe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createICmpLt(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ICmpLt, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createICmpLe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ICmpLe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUCmpGt(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::UCmpGt, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUCmpGe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::UCmpGe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUCmpLt(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::UCmpLt, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUCmpLe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::UCmpLe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFCmpEq(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FCmpEq, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFCmpNe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FCmpNe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFCmpGt(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FCmpGt, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFCmpGe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FCmpGe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFCmpLt(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FCmpLt, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFCmpLe(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FCmpLe, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createLeftShift(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::ShiftLeft, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createLRightShift(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::LShiftRight, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createARightShift(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::AShiftRight, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createAnd(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::And, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createXor(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::Xor, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createOr(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::Or, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createPhi(Register* reg, std::vector<Node*> values) {
    auto ins = std::make_unique<Instruction>(Node::NodeKind::Phi, reg);
    auto retPtr = ins.get();
    for(auto value : values)
        ins->addOperand(value);
    insert(std::move(ins));
    return retPtr;
}

Node* Builder::createGEP(Register* reg, Node* ptr, std::vector<Node*> indices) {
    auto ins = std::make_unique<Instruction>(Node::NodeKind::GEP, reg, ptr);
    auto retPtr = ins.get();
    for(auto index : indices)
        ins->addOperand(index);
    insert(std::move(ins));
    return retPtr;
}

Node* Builder::createZext(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Zext, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createSext(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Sext, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createTrunc(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Trunc, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFptrunc(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Fptrunc, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFpext(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Fpext, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFptosi(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Fptosi, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFptoui(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Fptoui, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createSitofp(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Sitofp, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUitofp(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Uitofp, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createPtrtoint(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Ptrtoint, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createInttoptr(Register* result, Node* value, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::Inttoptr, result, value, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createIDiv(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::IDiv, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUDiv(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::UDiv, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFDiv(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FDiv, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createIRem(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::IRem, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createURem(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::URem, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createIMul(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::IMul, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createUMul(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::UMul, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createFMul(Node* lhs, Node* rhs, Register* reg) {
    auto ret = std::make_unique<Instruction>(Node::NodeKind::FMul, reg, lhs, rhs);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}

Node* Builder::createGenericCast(Register* reg, Node* from, Type* toType) {
    auto ret = std::make_unique<Cast>(Node::NodeKind::GenericCast, reg, from, toType);
    auto retPtr = ret.get();
    insert(std::move(ret));
    return retPtr;
}
}