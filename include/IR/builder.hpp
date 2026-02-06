#pragma once

#include "IR/folder.hpp"
#include "IR/value.hpp"

namespace scbe::IR {

class Block;
class GlobalVariable;

class Builder {
public:
    Builder(Ref<Context> context) : m_context(context), m_folder(context) {}

    void setInsertPoint(Instruction* instruction) { m_insertPoint = instruction; }
    void resetInsertPoint() { m_insertPoint = nullptr; }
    void setCurrentBlock(Block* block) { m_currentBlock = block; resetInsertPoint(); }

    Instruction* getInsertPoint() const { return m_insertPoint; }
    Block* getCurrentBlock() const { return m_currentBlock; }

    AllocateInstruction* createAllocate(Type* type, const std::string& name = "");
    Value* createLoad(Value* ptr, const std::string& name = "");
    Value* createStore(Value* ptr, Value* value);
    Value* createAdd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createSub(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createIMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFMul(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createIDiv(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUDiv(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFDiv(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createIRem(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createURem(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpEq(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpNe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpGt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpGe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpLt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createICmpLe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUCmpGt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUCmpGe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUCmpLt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createUCmpLe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpEq(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpNe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpGt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpGe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpLt(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createFCmpLe(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createLeftShift(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createLRightShift(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createARightShift(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createAnd(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createOr(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createXor(Value* lhs, Value* rhs, const std::string& name = "");
    Value* createGEP(Value* ptr, const std::vector<Value*>& indices, const std::string& name = "");
    Value* createCall(Value* callee, const std::string& name = "");
    Value* createCall(Value* callee, const std::vector<Value*>& args, const std::string& name = "");
    Value* createZext(Value* value, Type* toType, const std::string& name = "");
    Value* createSext(Value* value, Type* toType, const std::string& name = "");
    Value* createTrunc(Value* value, Type* toType, const std::string& name = "");
    Value* createFptrunc(Value* value, Type* toType, const std::string& name = "");
    Value* createFpext(Value* value, Type* toType, const std::string& name = "");
    Value* createFptosi(Value* value, Type* toType, const std::string& name = "");
    Value* createFptoui(Value* value, Type* toType, const std::string& name = "");
    Value* createSitofp(Value* value, Type* toType, const std::string& name = "");
    Value* createUitofp(Value* value, Type* toType, const std::string& name = "");
    Value* createBitcast(Value* value, Type* toType, const std::string& name = "");
    Value* createPtrtoint(Value* value, Type* toType, const std::string& name = "");
    Value* createInttoptr(Value* value, Type* toType, const std::string& name = "");
    Value* createExtractValue(Value* from, ConstantInt* index, const std::string& name = "");
    Value* createPhi(const std::vector<std::pair<Value*, Block*>>& values, Type* type, const std::string& name = "");

    void createRet(Value* value = nullptr);
    void createCondJump(Block* first, Block* second, Value* cond);
    void createJump(Block* to);
    void createSwitch(Value* value, Block* defaultBlock, const std::vector<std::pair<ConstantInt*, Block*>>& cases);

    void setInsertBefore(bool insertBefore) { m_insertBefore = insertBefore; }

private:
    void insert(std::unique_ptr<Instruction> instruction);

private:
    Ref<Context> m_context;
    Block* m_currentBlock = nullptr;
    Instruction* m_insertPoint = nullptr;
    bool m_insertBefore = false;

    Folder m_folder;
};

}