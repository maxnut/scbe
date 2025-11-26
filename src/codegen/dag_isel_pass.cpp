#include "codegen/dag_isel_pass.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/instruction.hpp"
#include "IR/value.hpp"
#include "ISel/DAG/instruction.hpp"
#include "ISel/DAG/value.hpp"
#include "MIR/function.hpp"
#include "target/instruction_info.hpp"
#include "target/x64/x64_patterns.hpp"
#include "unit.hpp"

#include <algorithm>
#include <deque>
#include <utility>

namespace scbe::Codegen {

void createMirBlocks(IR::Function* function) {
    for(auto& current : function->getBlocks()) {
        std::unique_ptr<MIR::Block> mirBlock = std::make_unique<MIR::Block>(current->getName(), current.get());
        function->getMachineFunction()->addBlock(std::move(mirBlock));
    }

    for(auto& current : function->getBlocks()) {
        MIR::Block* mirBlock = current->getMIRBlock();
        for(auto& pair : current->getSuccessors())
            mirBlock->addSuccessor(pair.first->getMIRBlock());
        for(auto& pair : current->getPredecessors())
            mirBlock->addPredecessor(pair.first->getMIRBlock());
    }
}

void DagISelPass::init(Unit& unit) {
    for(auto& function : unit.m_functions) {
        auto machine = std::make_unique<MIR::Function>(function->getName(), function.get(), m_registerInfo);
        function->setMachineFunction(std::move(machine));
    }
}

bool DagISelPass::run(IR::Function* function) {
    m_builder.setRoot(nullptr);
    m_output = nullptr;
    m_constantIdx = 0;
    m_dagRootToMirNodes.clear();
    m_nodesToMIROperands.clear();
    m_valuesToNodes.clear();
    m_bestMatch.clear();
    m_inProgress.clear();
    m_registers.clear();
    m_constantInts.clear();
    m_constantFloats.clear();
    m_frameIndices.clear();
    m_roots.clear();

    m_output = function->getMachineFunction();
    if(!buildDAG(function)) {
        return false;
    }

    createMirBlocks(function);

    // first fill map
    for(auto& block : function->getBlocks()) {
        auto dagRoot = cast<ISel::DAG::Root>(m_valuesToNodes[block.get()]);
        m_dagRootToMirNodes.emplace(dagRoot, block->getMIRBlock());
    }

    for(auto& block : function->getBlocks()) {
        MIR::Block* mirBlock = block->getMIRBlock();
        auto dagRoot = cast<ISel::DAG::Root>(m_valuesToNodes[block.get()]);
        auto next = dagRoot->getNext();
        while(next) {
            selectPattern(next);
            emitOrGet(next, mirBlock);
            next = next->getNext();
        }
    }
    return false;
}

bool DagISelPass::buildDAG(IR::Function* function) {
    for(auto& block : function->getBlocks()) {
        auto root = m_builder.createRoot(block->getName());
        m_valuesToNodes[block.get()] = root.get();    
        m_roots.push_back(std::move(root));
    }
    
    for(auto& block : function->getBlocks()) {
        if(!buildBlock(block.get()))
            return false;
    }
    
    return true;
}

ISel::DAG::Root* DagISelPass::buildBlock(IR::Block* block) {
    auto prevRoot = m_builder.getRoot();
    std::vector<std::pair<IR::Instruction*, ISel::DAG::Chain*>> chains;
    ISel::DAG::Root* root = cast<ISel::DAG::Root>(m_valuesToNodes[block]);
    m_builder.setRoot(root);
    ISel::DAG::Chain* current = root;
    for(auto& instruction : block->getInstructions()) {
        if(isChain(instruction.get())) {
            auto chain = earlyBuildChain(instruction.get());
            chains.push_back({instruction.get(), chain});
            current->setNext(chain);
            current = chain;
        }
    }

    for(auto& pair : chains) {
        patchChain(pair.first, pair.second);
    }
    m_builder.setRoot(prevRoot);
    return root;
}

ISel::DAG::Chain* DagISelPass::earlyBuildChain(IR::Instruction* instruction) {
    std::unique_ptr<ISel::DAG::Chain> ret = nullptr;
    switch(instruction->getOpcode()) {
        case IR::Instruction::Opcode::Ret: {
            ret = std::make_unique<ISel::DAG::Chain>(ISel::DAG::Node::NodeKind::Ret);
            break;
        }
        case IR::Instruction::Opcode::Jump: {
            ret = std::make_unique<ISel::DAG::Chain>(ISel::DAG::Node::NodeKind::Jump);
            break;
        }
        case IR::Instruction::Opcode::Load: {
            ISel::DAG::Value* retValue = nullptr;
            if(instruction->getType()->isStructType()) {
                auto multi = std::make_unique<ISel::DAG::MultiValue>(instruction->getType());
                retValue = multi.get();
                std::deque<StructType*> worklist; worklist.push_back(cast<StructType>(instruction->getType()));
                while(!worklist.empty()) {
                    auto type = worklist.front();
                    worklist.pop_front();
                    for(size_t i = 0; i < type->getContainedTypes().size(); i++) {
                        Type* field = type->getContainedTypes()[i];
                        auto reg = std::make_unique<ISel::DAG::Register>(instruction->getName() + "_" + std::to_string(i), field);
                        multi->addValue(reg.get());
                        m_builder.insert(std::move(reg));
                        if(!field->isStructType()) continue;
                        worklist.push_back(cast<StructType>(field));
                    }
                }
                m_builder.insert(std::move(multi));
            }
            else {
                retValue = makeOrGetRegister(instruction, instruction->getType());
            }
            ret = std::make_unique<ISel::DAG::Chain>(ISel::DAG::Node::NodeKind::Load, retValue);
            m_valuesToNodes[instruction] = retValue;
            break;
        }
        case IR::Instruction::Opcode::Store: {
            ret = std::make_unique<ISel::DAG::Chain>(ISel::DAG::Node::NodeKind::Store);
            break;
        }
        case IR::Instruction::Opcode::Call: {
            auto call = cast<IR::CallInstruction>(instruction);
            ISel::DAG::Value* result = nullptr;
            if(instruction->getType()->isStructType()) {
                auto multi = std::make_unique<ISel::DAG::MultiValue>(instruction->getType());
                result = multi.get();
                std::deque<StructType*> worklist; worklist.push_back(cast<StructType>(instruction->getType()));
                while(!worklist.empty()) {
                    auto type = worklist.front();
                    worklist.pop_front();
                    for(size_t i = 0; i < type->getContainedTypes().size(); i++) {
                        Type* field = type->getContainedTypes()[i];
                        auto reg = std::make_unique<ISel::DAG::Register>(instruction->getName() + "_" + std::to_string(i), field);
                        multi->addValue(reg.get());
                        m_builder.insert(std::move(reg));
                        if(!field->isStructType()) continue;
                        worklist.push_back(cast<StructType>(field));
                    }
                }
                m_builder.insert(std::move(multi));
            }
            else {
                result = makeOrGetRegister(instruction, instruction->getType());
            }
            ret = std::make_unique<ISel::DAG::Call>(result);
            m_valuesToNodes[instruction] = result;
            break;
        }
        case IR::Instruction::Opcode::Switch: {
            ret = std::make_unique<ISel::DAG::Chain>(ISel::DAG::Node::NodeKind::Switch);
            break;
        }
        case IR::Instruction::Opcode::Phi: {
            auto node = makeOrGetRegister(instruction, instruction->getType());
            ret = std::make_unique<ISel::DAG::Chain>(ISel::DAG::Node::NodeKind::Phi, node);
            m_valuesToNodes[instruction] = node;
            break;
        }
        default:
            throw std::runtime_error("Unsupported opcode " + std::to_string((uint32_t)instruction->getOpcode()));
    }
    auto ptr = ret.get();
    m_builder.insert(std::move(ret));
    return ptr;
}

void DagISelPass::patchChain(IR::Instruction* instruction, ISel::DAG::Chain* chain) {
    switch(chain->getKind()) {
        case ISel::DAG::Node::NodeKind::Ret: {
            if(instruction->getNumOperands() > 0) {
                auto value = buildNonChain(instruction->getOperand(0));
                chain->addOperand(value);
            }
            return;
        }
        case ISel::DAG::Node::NodeKind::Jump: {
            auto block1 = cast<ISel::DAG::Root>(buildNonChain(instruction->getOperand(0)));
            chain->addOperand(block1);
            if(instruction->getNumOperands() > 1) {
                auto block2 = cast<ISel::DAG::Root>(buildNonChain(instruction->getOperand(1)));
                auto cond = buildNonChain(instruction->getOperand(2));
                chain->addOperand(block2);
                chain->addOperand(cond);
            }
            return;
        }
        case ISel::DAG::Node::NodeKind::Load: {
            auto value = cast<ISel::DAG::FrameIndex>(buildNonChain(instruction->getOperand(0)));
            chain->addOperand(value);
            return;
        }
        case ISel::DAG::Node::NodeKind::Store: {
            auto ptr = cast<ISel::DAG::FrameIndex>(buildNonChain(instruction->getOperand(0)));
            auto value = buildNonChain(instruction->getOperand(1));
            chain->addOperand(ptr);
            chain->addOperand(value);
            return;
        }
        case ISel::DAG::Node::NodeKind::Call: {
            auto call = cast<IR::CallInstruction>(instruction);
            auto callee = buildNonChain(call->getCallee());
            chain->addOperand(callee);
            for(auto& operand : call->getArguments()) {
                auto value = buildNonChain(operand);
                chain->addOperand(value);
            }
            cast<ISel::DAG::Call>(chain)->setResultUsed(call->getUses().size() > 0);
            return;
        }
        case ISel::DAG::Node::NodeKind::Switch: {
            IR::SwitchInstruction* switchInstruction = cast<IR::SwitchInstruction>(instruction);
            ISel::DAG::Node* condition = buildNonChain(switchInstruction->getCondition());
            ISel::DAG::Root* defaultCase = cast<ISel::DAG::Root>(buildNonChain(switchInstruction->getDefaultCase()));
            chain->addOperand(condition);
            chain->addOperand(defaultCase);
            for(auto casePair : switchInstruction->getCases()) {
                ISel::DAG::Root* block = cast<ISel::DAG::Root>(buildNonChain(casePair.second));
                chain->addOperand(buildNonChain(casePair.first));
                chain->addOperand(block);
            }
            return;
        }
        case ISel::DAG::Node::NodeKind::Phi: {
            std::vector<ISel::DAG::Node*> values;

            auto phi = cast<IR::PhiInstruction>(instruction);
            for(size_t i = 0; i < phi->getNumOperands(); i++) {
                auto operand = phi->getOperand(i);
                auto value = buildNonChain(operand);
                values.push_back(value);
            }

            for(auto value : values)
                chain->addOperand(value);

            return;
        }
        default:
            break;
    }
    throw std::runtime_error("Unsupported instruction");
}

ISel::DAG::Node* DagISelPass::buildNonChain(IR::Value* value) {
    if(m_valuesToNodes.contains(value))
        return m_valuesToNodes[value];

    switch (value->getKind()) {
        case IR::Value::ValueKind::ConstantInt: {
            IR::ConstantInt* constantInt = (IR::ConstantInt*)value;
            auto node = makeOrGetConstInt(constantInt->getValue(), constantInt->getType());
            m_valuesToNodes[value] = node;
            return node;
        }
        case IR::Value::ValueKind::ConstantFloat: {
            // TODO remove loadconstant. just handle them at the MIR level depending on arch
            IR::ConstantFloat* constantFloat = (IR::ConstantFloat*)value;
            auto node = makeOrGetConstFloat(constantFloat->getValue(), constantFloat->getType());
            auto ll = std::make_unique<ISel::DAG::Instruction>(
                ISel::DAG::Node::NodeKind::LoadConstant,
                makeOrGetRegister(value, value->getType()),
                node
            );
            auto ret = ll.get();
            m_valuesToNodes[value] = ret;
            m_builder.insert(std::move(ll));
            return ret;
        }
        case IR::Value::ValueKind::FunctionArgument: {
            IR::FunctionArgument* functionArgument = (IR::FunctionArgument*)value;
            
            if(functionArgument->hasFlag(IR::Value::Flag::ByVal)) {
                Type* sizeType = value->getType()->isPtrType() ? cast<PointerType>(value->getType())->getPointee() : value->getType();
                size_t size = m_dataLayout->getSize(sizeType);
                int64_t argumentStackOffset = -16;
                for(size_t i = 0; i < functionArgument->getSlot(); i++) {
                    auto& arg = m_output->getIRFunction()->getArguments().at(i);
                    if(!arg->hasFlag(IR::Value::Flag::ByVal)) continue;
                    size_t argSize = m_dataLayout->getSize(cast<PointerType>(arg->getType())->getPointee());
                    argumentStackOffset -= argSize;
                }
                m_output->getStackFrame().addStackSlot(size, argumentStackOffset, m_dataLayout->getAlignment(sizeType));
                auto node = makeOrGetFrameIndex(m_output->getStackFrame().getNumStackSlots() - 1, value->getType());
                m_valuesToNodes[value] = node;
                return node;
            }

            auto node = std::make_unique<ISel::DAG::FunctionArgument>(functionArgument->getSlot(), functionArgument->getType());
            auto ret = node.get();
            m_valuesToNodes[value] = ret;
            m_builder.insert(std::move(node));
            return ret;
        }
        case IR::Value::ValueKind::GlobalVariable: {
            IR::GlobalVariable* global = cast<IR::GlobalVariable>(value);
            auto globalValue = std::make_unique<ISel::DAG::GlobalValue>(global);
            auto globalValuePtr = globalValue.get();
            m_builder.insert(std::move(globalValue));
            auto node = std::make_unique<ISel::DAG::Instruction>(
                ISel::DAG::Node::NodeKind::LoadGlobal,
                makeOrGetRegister(global, value->getType()),
                globalValuePtr
            );
            auto ptr = node.get();
            m_valuesToNodes[value] = ptr;
            m_builder.insert(std::move(node));
            return ptr;
        }
        case IR::Value::ValueKind::Function: {
            IR::Function* func = cast<IR::Function>(value);
            auto globalValue = std::make_unique<ISel::DAG::Function>(func);
            auto globalValuePtr = globalValue.get();
            m_builder.insert(std::move(globalValue));
            auto node = std::make_unique<ISel::DAG::Instruction>(
                ISel::DAG::Node::NodeKind::LoadGlobal,
                makeOrGetRegister(func, value->getType()),
                globalValuePtr
            );
            auto ptr = node.get();
            m_valuesToNodes[value] = ptr;
            m_builder.insert(std::move(node));
            return ptr;
        }
        case IR::Value::ValueKind::Register: {
            auto ret = buildInstruction(cast<IR::Instruction>(value));
            if(!ret) return nullptr;
            m_valuesToNodes[value] = ret;
            return ret;
        }
        case IR::Value::ValueKind::Block:
            return buildBlock(cast<IR::Block>(value));
        case IR::Value::ValueKind::UndefValue: {
            IR::UndefValue* undef = (IR::UndefValue*)value;
            auto node = buildNonChain(IR::Constant::getZeroInitalizer(undef->getType(), m_dataLayout, m_context));
            m_valuesToNodes[value] = node;
            return node;
        }
        default:
            break;
    }

    throw std::runtime_error("Unsupported value " + value->getName());
}

#define GET_BINOP(instr) \
    auto lhs = buildNonChain(value->getOperand(0)); \
    auto rhs = buildNonChain(value->getOperand(1)); \
    auto node = m_builder.create##instr(lhs, rhs, makeOrGetRegister(value, value->getType())); \
    cast<ISel::DAG::Instruction>(node)->setChainIndex(chainIndex); \
    m_valuesToNodes[value] = node; \
    return node;

#define GET_CAST(instr) \
    IR::CastInstruction* irCast = cast<IR::CastInstruction>(value); \
    ISel::DAG::Node* operand = buildNonChain(irCast->getOperand(0)); \
    ISel::DAG::Register* result = makeOrGetRegister(value, value->getType()); \
    auto node = m_builder.create##instr(result, operand, irCast->getType()); \
    cast<ISel::DAG::Instruction>(node)->setChainIndex(chainIndex); \
    m_valuesToNodes[value] = node; \
    return node;


ISel::DAG::Node* DagISelPass::buildInstruction(IR::Instruction* value) {
    if(m_valuesToNodes.contains(value))
        return m_valuesToNodes[value];

    IR::Block* block = value->getParentBlock();
    size_t chainIndex = 0;
    for(auto& ins : block->getInstructions()) {
        if(ins.get() == value) break;
        if(isChain(ins.get())) chainIndex++;
    }

    switch (value->getOpcode()) {
        case IR::Instruction::Opcode::Allocate: {
            Type* sizeType = value->getType()->isPtrType() ? cast<PointerType>(value->getType())->getPointee() : value->getType();
            m_output->getStackFrame().addStackSlot(m_dataLayout->getSize(sizeType), m_dataLayout->getAlignment(sizeType));
            auto node = makeOrGetFrameIndex(m_output->getStackFrame().getNumStackSlots() - 1, value->getType());
            m_valuesToNodes[value] = node;
            return node;
        }
        case IR::Instruction::Opcode::Call:
        case IR::Instruction::Opcode::Phi:
        case IR::Instruction::Opcode::Load: {
            auto node = makeOrGetRegister(value, value->getType());
            m_valuesToNodes[value] = node;
            return node;
        }
        case IR::Instruction::Opcode::Add: {
            GET_BINOP(Add);
        }
        case IR::Instruction::Opcode::Sub: {
            GET_BINOP(Sub);
        }
        case IR::Instruction::Opcode::IDiv: {
            GET_BINOP(IDiv);
        }
        case IR::Instruction::Opcode::UDiv: {
            GET_BINOP(UDiv);
        }
        case IR::Instruction::Opcode::FDiv: {
            GET_BINOP(FDiv);
        }
        case IR::Instruction::Opcode::IMul: {
            GET_BINOP(IMul);
        }
        case IR::Instruction::Opcode::UMul: {
            GET_BINOP(UMul);
        }
        case IR::Instruction::Opcode::FMul: {
            GET_BINOP(FMul);
        }
        case IR::Instruction::Opcode::IRem: {
            GET_BINOP(IRem);
        }
        case IR::Instruction::Opcode::URem: {
            GET_BINOP(URem);
        }
        case IR::Instruction::Opcode::ICmpEq: {
            GET_BINOP(ICmpEq);
        }
        case IR::Instruction::Opcode::ICmpNe: {
            GET_BINOP(ICmpNe);
        }
        case IR::Instruction::Opcode::ICmpGt: {
            GET_BINOP(ICmpGt);
        }
        case IR::Instruction::Opcode::ICmpGe: {
            GET_BINOP(ICmpGe);
        }
        case IR::Instruction::Opcode::ICmpLt: {
            GET_BINOP(ICmpLt);
        }
        case IR::Instruction::Opcode::ICmpLe: {
            GET_BINOP(ICmpLe);
        }
        case IR::Instruction::Opcode::UCmpGt: {
            GET_BINOP(UCmpGt);
        }
        case IR::Instruction::Opcode::UCmpGe: {
            GET_BINOP(UCmpGe);
        }
        case IR::Instruction::Opcode::UCmpLt: {
            GET_BINOP(UCmpLt);
        }
        case IR::Instruction::Opcode::UCmpLe: {
            GET_BINOP(UCmpLe);
        }
        case IR::Instruction::Opcode::FCmpEq: {
            GET_BINOP(FCmpEq);
        }
        case IR::Instruction::Opcode::FCmpNe: {
            GET_BINOP(FCmpNe);
        }
        case IR::Instruction::Opcode::FCmpGt: {
            GET_BINOP(FCmpGt);
        }
        case IR::Instruction::Opcode::FCmpGe: {
            GET_BINOP(FCmpGe);
        }
        case IR::Instruction::Opcode::FCmpLt: {
            GET_BINOP(FCmpLt);
        }
        case IR::Instruction::Opcode::FCmpLe: {
            GET_BINOP(FCmpLe);
        }
        case IR::Instruction::Opcode::ShiftLeft: {
            GET_BINOP(LeftShift);
        }
        case IR::Instruction::Opcode::LShiftRight: {
            GET_BINOP(LRightShift);
        }
        case IR::Instruction::Opcode::AShiftRight: {
            GET_BINOP(ARightShift);
        }
        case IR::Instruction::Opcode::And: {
            GET_BINOP(And);
        }
        case IR::Instruction::Opcode::Or: {
            GET_BINOP(Or);
        }
        case IR::Instruction::Opcode::Xor: {
            GET_BINOP(Xor);
        }
        case IR::Instruction::Opcode::GetElementPtr: {
            auto gep = cast<IR::GEPInstruction>(value);
            ISel::DAG::Register* node = makeOrGetRegister(value, value->getType());
            auto ptr = buildNonChain(gep->getPointer());
            std::vector<ISel::DAG::Node*> indices;
            for(size_t i = 1; i < gep->getNumOperands(); i++) {
                auto operand = gep->getOperand(i);
                auto value = buildNonChain(operand);
                indices.push_back(value);
            }
            auto gepNode = m_builder.createGEP(node, ptr, indices);
            cast<ISel::DAG::Instruction>(gepNode)->setChainIndex(chainIndex);
            m_valuesToNodes[value] = gepNode;
            return gepNode;
        }
        case IR::Instruction::Opcode::Zext: {
            GET_CAST(Zext);
        }
        case IR::Instruction::Opcode::Sext: {
            GET_CAST(Sext);
        }
        case IR::Instruction::Opcode::Trunc: {
            GET_CAST(Trunc);
        }
        case IR::Instruction::Opcode::Fptrunc: {
            GET_CAST(Fptrunc);
        }
        case IR::Instruction::Opcode::Fpext: {
            GET_CAST(Fpext);
        }
        case IR::Instruction::Opcode::Fptosi: {
            GET_CAST(Fptosi);
        }
        case IR::Instruction::Opcode::Fptoui: {
            GET_CAST(Fptoui);
        }
        case IR::Instruction::Opcode::Uitofp: {
            GET_CAST(Uitofp);
        }
        case IR::Instruction::Opcode::Sitofp: {
            GET_CAST(Sitofp);
        }
        case IR::Instruction::Opcode::ExtractValue: {
            auto extract = cast<IR::ExtractValueInstruction>(value);
            // we expect multivalue here
            ISel::DAG::MultiValue* operand = cast<ISel::DAG::MultiValue>(buildNonChain(extract->getAggregate()));
            ISel::DAG::Node* res = operand->getValues().at(extract->getIndex()->getValue());
            m_valuesToNodes[value] = res;
            return res;
        }
        // these are only a type cast
        case IR::Instruction::Opcode::Ptrtoint:
        case IR::Instruction::Opcode::Inttoptr:
        case IR::Instruction::Opcode::Bitcast: {
            GET_CAST(GenericCast);
        }
        case IR::Instruction::Opcode::Jump:
        case IR::Instruction::Opcode::Ret:
        case IR::Instruction::Opcode::Store:
        case IR::Instruction::Opcode::Switch:
        case IR::Instruction::Opcode::Count:
            break;
    }
    throw std::runtime_error("Unsupported instruction for " + value->getName());
}

void DagISelPass::selectPattern(ISel::DAG::Node* node) {
    if(m_bestMatch.contains(node)) return;

    if(node->getKind() == ISel::DAG::Node::NodeKind::ConstantFloat || node->getKind() == ISel::DAG::Node::NodeKind::GlobalValue) return; // TODO remove this eventually
    else if(node->getKind() == ISel::DAG::Node::NodeKind::MultiValue) {
        for(auto value : cast<ISel::DAG::MultiValue>(node)->getValues())
            selectPattern(value);
    }
    m_bestMatch[node] = MatchResult(nullptr, 0, node); // temporary for recursion

    if(auto instruction = dyn_cast<ISel::DAG::Instruction>(node)) {
        for(auto& op : instruction->getOperands())
            selectPattern(op);
        if(instruction->getResult()) selectPattern(instruction->getResult());
    }

    auto& patterns = m_instructionInfo->getPatterns(node->getKind());
    if(patterns.empty()) {
        m_bestMatch.erase(node);
        return;
    }
    
    std::vector<MatchResult> results;
    for(auto& pattern : patterns) {
        if(!pattern.match(node, m_dataLayout)) continue;
        uint32_t cost = pattern.m_baseCost;
        if(auto instruction = dyn_cast<ISel::DAG::Instruction>(node)) {
            for(size_t i = 0; i < instruction->getOperands().size(); i++) {
                if(std::find(pattern.m_coveredOperands.begin(), pattern.m_coveredOperands.end(), i) != pattern.m_coveredOperands.end()
                    || !m_bestMatch.contains(instruction->getOperands().at(i)))
                    continue;
                cost += m_bestMatch.at(instruction->getOperands().at(i)).m_cost;
            }
        }
        results.push_back(MatchResult(&pattern, cost, node));
    }

    if(results.empty())
        throw std::runtime_error("No patterns matched for " + std::to_string((uint32_t)node->getKind()));

    auto best = *std::min_element(results.begin(), results.end(), [](const MatchResult& a, const MatchResult& b) {
        return a.m_cost < b.m_cost;
    });

    m_bestMatch[node] = best;
}

MIR::Operand* DagISelPass::emitOrGet(ISel::DAG::Node* node, MIR::Block* block) {
    if(m_nodesToMIROperands.contains(node)) return m_nodesToMIROperands[node];

    if(auto ins = dyn_cast<ISel::DAG::Instruction>(node)) {
        if(ins->getResult()) m_nodesToMIROperands[node] = emitOrGet(ins->getResult(), block); // store early to avoid recursion
        ISel::DAG::Root* root = cast<ISel::DAG::Root>(m_valuesToNodes.at(block->getIRBlock()));
        ISel::DAG::Chain* chain = root;
        for(size_t i = 0; i < ins->getChainIndex(); i++) chain = chain->getNext();

        if(chain != root) {
            selectPattern(chain);
            emitOrGet(chain, block);
        }
    }

    if(!m_bestMatch.contains(node))
        throw std::runtime_error("No patterns matched for " + std::to_string((uint32_t)node->getKind()));

    auto& match = m_bestMatch.at(node);
    auto res = match.m_pattern->emit(block, m_dataLayout, m_instructionInfo, node, this, m_context);

    m_nodesToMIROperands[node] = res;
    return res;
}

bool DagISelPass::isChain(IR::Instruction* instruction) {
    return instruction->getOpcode() == IR::Instruction::Opcode::Ret
        || instruction->getOpcode() == IR::Instruction::Opcode::Jump
        || instruction->getOpcode() == IR::Instruction::Opcode::Load
        || instruction->getOpcode() == IR::Instruction::Opcode::Store
        || instruction->getOpcode() == IR::Instruction::Opcode::Switch
        || instruction->getOpcode() == IR::Instruction::Opcode::Phi
        || instruction->getOpcode() == IR::Instruction::Opcode::Call;
}


ISel::DAG::Register* DagISelPass::makeOrGetRegister(IR::Value* referenceValue, Type* type) {
    if(m_registers.contains(referenceValue)) return m_registers[referenceValue];
    auto reg = std::make_unique<ISel::DAG::Register>(referenceValue->getName(), type);
    auto ret = reg.get();
    m_registers[referenceValue] = ret;
    m_builder.insert(std::move(reg));
    return ret;
}

ISel::DAG::ConstantInt* DagISelPass::makeOrGetConstInt(int64_t value, Type* type) {
    size_t hash = std::hash<int64_t>{}(value) ^ std::hash<Type*>{}(type);
    if(m_constantInts.contains(hash)) return m_constantInts[hash];
    auto constant = std::make_unique<ISel::DAG::ConstantInt>(value, type);
    auto ret = constant.get();
    m_constantInts[hash] = ret;
    m_builder.insert(std::move(constant));
    return ret;
}

ISel::DAG::ConstantFloat* DagISelPass::makeOrGetConstFloat(double value, Type* type) {
    size_t hash = std::hash<double>{}(value) ^ std::hash<Type*>{}(type);
    if(m_constantFloats.contains(hash)) return m_constantFloats[hash];
    auto constant = std::make_unique<ISel::DAG::ConstantFloat>(value, type);
    auto ret = constant.get();
    m_constantFloats[hash] = ret;
    m_builder.insert(std::move(constant));
    return ret;
}

ISel::DAG::FrameIndex* DagISelPass::makeOrGetFrameIndex(uint32_t slot, Type* type) {
    size_t hash = std::hash<uint32_t>{}(slot) ^ std::hash<Type*>{}(type);
    if(m_frameIndices.contains(hash)) return m_frameIndices[hash];
    auto frameIndex = std::make_unique<ISel::DAG::FrameIndex>(slot, type);
    auto ret = frameIndex.get();
    m_frameIndices[hash] = ret;
    m_builder.insert(std::move(frameIndex));
    return ret;
}
    
}