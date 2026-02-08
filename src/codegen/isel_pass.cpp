#include "codegen/isel_pass.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "IR/instruction.hpp"
#include "IR/value.hpp"
#include "ISel/instruction.hpp"
#include "ISel/node.hpp"
#include "ISel/value.hpp"
#include "MIR/function.hpp"
#include "target/instruction_info.hpp"
#include "target/instruction_utils.hpp"
#include "unit.hpp"
#include "hash.hpp"

#include <deque>

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

void ISelPass::init(Unit& unit) {
    for(auto& function : unit.getFunctions()) {
        auto machine = std::make_unique<MIR::Function>(function->getName(), function.get(), m_registerInfo);
        function->setMachineFunction(std::move(machine));
    }
}

bool ISelPass::run(IR::Function* function) {
    m_inserter.setRoot(nullptr);
    m_output = nullptr;
    m_constantIdx = 0;
    m_rootToMirNodes.clear();
    m_nodesToMIROperands.clear();
    m_valuesToNodes.clear();
    m_bestMatch.clear();
    m_registers.clear();
    m_constantInts.clear();
    m_constantFloats.clear();
    m_frameIndices.clear();
    m_roots.clear();

    createMirBlocks(function);

    m_output = function->getMachineFunction();

    if(!buildTree(function)) {
        return false;
    }

    for(auto& block : function->getBlocks()) {
        auto root = cast<ISel::Root>(m_valuesToNodes[block.get()]);
        m_rootToMirNodes.emplace(root, block->getMIRBlock());
    }

    for(auto& block : function->getBlocks()) {
        auto root = cast<ISel::Root>(m_valuesToNodes[block.get()]);
        for(ISel::Instruction* ins : root->m_instructions) {
            if(ins->getResult()) selectPattern(ins->getResult());
            selectPattern(ins);
        }
    }

    for(auto& block : function->getBlocks()) {
        auto root = cast<ISel::Root>(m_valuesToNodes[block.get()]);
        MIR::Block* mirBlock = block->getMIRBlock();
        for(ISel::Instruction* ins : root->m_instructions) {
            if(!m_bestMatch.contains(ins)) continue;
            emitOrGet(ins, mirBlock, false);
        }
    }

    return false;
}

bool ISelPass::buildTree(IR::Function* function) {
    for(auto& block : function->getBlocks()) {
        auto root = std::move(std::make_unique<ISel::Root>(block->getName()));
        m_valuesToNodes[block.get()] = root.get();    
        m_roots.push_back(std::move(root));
    }
    
    for(auto& block : function->getBlocks()) {
        auto prevRoot = m_inserter.getRoot();
        ISel::Root* root = cast<ISel::Root>(m_valuesToNodes[block.get()]);
        m_inserter.setRoot(root);
        for(auto& instruction : block->getInstructions())
            buildInstruction(instruction.get());

        m_inserter.setRoot(prevRoot);
    }

    for(auto& block : function->getBlocks()) {
        for(auto& instruction : block->getInstructions())
            patchInstruction(instruction.get());
    }
    
    return true;
}

ISel::Node* ISelPass::buildValue(IR::Value* value) {
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
            IR::ConstantFloat* constantFloat = (IR::ConstantFloat*)value;
            auto node = makeOrGetConstFloat(constantFloat->getValue(), constantFloat->getType());
            m_valuesToNodes[value] = node;
            return node;
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

            auto node = std::make_unique<ISel::FunctionArgument>(functionArgument->getSlot(), functionArgument->getType());
            auto ret = node.get();
            m_valuesToNodes[value] = ret;
            m_inserter.insert(std::move(node));
            return ret;
        }
        case IR::Value::ValueKind::GlobalVariable: {
            IR::GlobalVariable* global = cast<IR::GlobalVariable>(value);
            auto globalValue = std::make_unique<ISel::GlobalValue>(global);
            auto ptr = globalValue.get();
            m_inserter.insert(std::move(globalValue));
            m_valuesToNodes[value] = ptr;
            return ptr;
        }
        case IR::Value::ValueKind::Function: {
            IR::Function* func = cast<IR::Function>(value);
            auto globalValue = std::make_unique<ISel::GlobalValue>(func);
            auto ptr = globalValue.get();
            m_inserter.insert(std::move(globalValue));
            m_valuesToNodes[value] = ptr;
            return ptr;
        }
        case IR::Value::ValueKind::UndefValue: {
            IR::UndefValue* undef = (IR::UndefValue*)value;
            auto node = buildValue(IR::Constant::getZeroInitalizer(undef->getType(), m_dataLayout, m_context));
            m_valuesToNodes[value] = node;
            return node;
        }
        case IR::Value::ValueKind::NullValue: {
            IR::NullValue* null = (IR::NullValue*)value;
            auto node = buildValue(m_context->getConstantInt(m_dataLayout->getPointerSize() * 8, 0));
            m_valuesToNodes[value] = node;
            return node;
        }
        case IR::Value::ValueKind::Register: {
            return buildInstruction(cast<IR::Instruction>(value));
        }
        default:
            break;
    }

    throw std::runtime_error("Unsupported value " + value->getName());
}

#define GET_BINOP(instr) \
    auto result = makeOrGetRegister(instruction, instruction->getType()); \
    auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::instr, result); \
    auto ret = ins.get(); \
    m_inserter.insert(std::move(ins)); \
    m_valuesToNodes[instruction] = ret; \
    m_inserter.setRoot(og); \
    return ret;

#define GET_CAST(instr) \
    auto result = makeOrGetRegister(instruction, instruction->getType()); \
    auto ins = std::make_unique<ISel::Cast>(ISel::Node::NodeKind::instr, result, instruction->getType()); \
    auto ret = ins.get(); \
    m_inserter.insert(std::move(ins)); \
    m_valuesToNodes[instruction] = ret; \
    m_inserter.setRoot(og); \
    return ret;


ISel::Node* ISelPass::buildInstruction(IR::Instruction* instruction) {
    if(m_valuesToNodes.contains(instruction))
        return m_valuesToNodes[instruction];

    auto og = m_inserter.getRoot();
    m_inserter.setRoot(cast<ISel::Root>(m_valuesToNodes.at(instruction->getParentBlock())));

    switch (instruction->getOpcode()) {
        case IR::Instruction::Opcode::Allocate: {
            Type* sizeType = instruction->getType()->isPtrType() ? cast<PointerType>(instruction->getType())->getPointee() : instruction->getType();
            m_output->getStackFrame().addStackSlot(m_dataLayout->getSize(sizeType), m_dataLayout->getAlignment(sizeType));
            auto node = makeOrGetFrameIndex(m_output->getStackFrame().getNumStackSlots() - 1, instruction->getType());
            m_valuesToNodes[instruction] = node;
            m_inserter.setRoot(og);
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
            GET_BINOP(ShiftLeft);
        }
        case IR::Instruction::Opcode::LShiftRight: {
            GET_BINOP(LShiftRight);
        }
        case IR::Instruction::Opcode::AShiftRight: {
            GET_BINOP(AShiftRight);
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
            auto gep = cast<IR::GEPInstruction>(instruction);
            ISel::Register* result = makeOrGetRegister(instruction, instruction->getType());
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::GEP, result);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
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
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::ExtractValue);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        // these are only a type cast
        case IR::Instruction::Opcode::Ptrtoint:
        case IR::Instruction::Opcode::Inttoptr:
        case IR::Instruction::Opcode::Bitcast: {
            GET_CAST(GenericCast);
        }
        case IR::Instruction::Opcode::Ret: {
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::Ret);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            return ret;
        }
        case IR::Instruction::Opcode::Jump: {
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::Jump);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        case IR::Instruction::Opcode::Load: {
            ISel::Value* retValue = nullptr;
            if(instruction->getType()->isStructType()) {
                auto multi = std::make_unique<ISel::MultiValue>(instruction->getType());
                retValue = multi.get();
                std::deque<StructType*> worklist; worklist.push_back(cast<StructType>(instruction->getType()));
                while(!worklist.empty()) {
                    auto type = worklist.front();
                    worklist.pop_front();
                    for(size_t i = 0; i < type->getContainedTypes().size(); i++) {
                        Type* field = type->getContainedTypes()[i];
                        auto reg = std::make_unique<ISel::Register>(instruction->getName() + "_" + std::to_string(i), field);
                        multi->addValue(reg.get());
                        m_inserter.insert(std::move(reg));
                        if(!field->isStructType()) continue;
                        worklist.push_back(cast<StructType>(field));
                    }
                }
                m_inserter.insert(std::move(multi));
            }
            else {
                retValue = makeOrGetRegister(instruction, instruction->getType());
            }
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::Load, retValue);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        case IR::Instruction::Opcode::Store: {
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::Store);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        case IR::Instruction::Opcode::Call: {
            auto call = cast<IR::CallInstruction>(instruction);
            ISel::Value* result = nullptr;
            if(instruction->getType()->isStructType()) {
                auto multi = std::make_unique<ISel::MultiValue>(instruction->getType());
                result = multi.get();
                std::deque<StructType*> worklist; worklist.push_back(cast<StructType>(instruction->getType()));
                while(!worklist.empty()) {
                    auto type = worklist.front();
                    worklist.pop_front();
                    for(size_t i = 0; i < type->getContainedTypes().size(); i++) {
                        Type* field = type->getContainedTypes()[i];
                        auto reg = std::make_unique<ISel::Register>(instruction->getName() + "_" + std::to_string(i), field);
                        multi->addValue(reg.get());
                        m_inserter.insert(std::move(reg));
                        if(!field->isStructType()) continue;
                        worklist.push_back(cast<StructType>(field));
                    }
                }
                m_inserter.insert(std::move(multi));
            }
            else if(!instruction->getType()->isVoidType()) {
                result = makeOrGetRegister(instruction, instruction->getType());
            }
            auto ins = std::make_unique<ISel::Call>(result, call->getCallingConvention());
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        case IR::Instruction::Opcode::Switch: {
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::Switch);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        case IR::Instruction::Opcode::Phi: {
            auto node = makeOrGetRegister(instruction, instruction->getType());
            auto ins = std::make_unique<ISel::Instruction>(ISel::Node::NodeKind::Phi, node);
            auto ret = ins.get();
            m_inserter.insert(std::move(ins));
            m_valuesToNodes[instruction] = ret;
            m_inserter.setRoot(og);
            return ret;
        }
        default:
            break;
    }
    throw std::runtime_error("Unsupported instruction for " + instruction->getName());
}

#define PATCH_BINOP() \
    ins->addOperand(buildValue(instruction->getOperand(0))); \
    ins->addOperand(buildValue(instruction->getOperand(1))); \
    m_inserter.setRoot(og); \
    return;

#define PATCH_CAST(instr) \
    IR::CastInstruction* irCast = cast<IR::CastInstruction>(instruction); \
    ins->addOperand(buildValue(irCast->getOperand(0))); \
    m_inserter.setRoot(og); \
    return;


void ISelPass::patchInstruction(IR::Instruction* instruction) {
    ISel::Instruction* ins = cast<ISel::Instruction>(m_valuesToNodes.at(instruction));

    auto og = m_inserter.getRoot();
    m_inserter.setRoot(cast<ISel::Root>(m_valuesToNodes.at(instruction->getParentBlock())));

    switch (instruction->getOpcode()) {
        case IR::Instruction::Opcode::Add: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::Sub: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::IDiv: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::UDiv: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FDiv: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::IMul: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::UMul: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FMul: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::IRem: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::URem: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ICmpEq: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ICmpNe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ICmpGt: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ICmpGe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ICmpLt: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ICmpLe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::UCmpGt: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::UCmpGe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::UCmpLt: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::UCmpLe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FCmpEq: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FCmpNe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FCmpGt: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FCmpGe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FCmpLt: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::FCmpLe: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::ShiftLeft: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::LShiftRight: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::AShiftRight: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::And: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::Or: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::Xor: {
            PATCH_BINOP();
        }
        case IR::Instruction::Opcode::GetElementPtr: {
            auto gep = cast<IR::GEPInstruction>(instruction);
            ins->addOperand(buildValue(gep->getPointer()));
            std::vector<ISel::Node*> indices;
            for(size_t i = 1; i < gep->getNumOperands(); i++) {
                auto operand = gep->getOperand(i);
                auto value = buildValue(operand);
                ins->addOperand(value);
            }
            m_inserter.setRoot(og);
            return;
        }
        case IR::Instruction::Opcode::Zext: {
            PATCH_CAST(Zext);
        }
        case IR::Instruction::Opcode::Sext: {
            PATCH_CAST(Sext);
        }
        case IR::Instruction::Opcode::Trunc: {
            PATCH_CAST(Trunc);
        }
        case IR::Instruction::Opcode::Fptrunc: {
            PATCH_CAST(Fptrunc);
        }
        case IR::Instruction::Opcode::Fpext: {
            PATCH_CAST(Fpext);
        }
        case IR::Instruction::Opcode::Fptosi: {
            PATCH_CAST(Fptosi);
        }
        case IR::Instruction::Opcode::Fptoui: {
            PATCH_CAST(Fptoui);
        }
        case IR::Instruction::Opcode::Uitofp: {
            PATCH_CAST(Uitofp);
        }
        case IR::Instruction::Opcode::Sitofp: {
            PATCH_CAST(Sitofp);
        }
        case IR::Instruction::Opcode::ExtractValue: {
            auto extract = cast<IR::ExtractValueInstruction>(instruction);
            // we expect multivalue here
            ins->addOperand((buildValue(extract->getAggregate())));
            ins->addOperand(buildValue(extract->getIndex()));
            m_inserter.setRoot(og);
            return;
        }
        // these are only a type cast
        case IR::Instruction::Opcode::Ptrtoint:
        case IR::Instruction::Opcode::Inttoptr:
        case IR::Instruction::Opcode::Bitcast: {
            PATCH_CAST(GenericCast);
        }
        case IR::Instruction::Opcode::Ret: {
            if(instruction->getNumOperands() > 0) {
                auto op = buildValue(instruction->getOperand(0));
                ins->addOperand(op);
            }
            return;
        }
        case IR::Instruction::Opcode::Jump: {
            auto block1 = cast<ISel::Root>(buildValue(instruction->getOperand(0)));
            ins->addOperand(block1);
            if(instruction->getNumOperands() > 1) {
                auto block2 = cast<ISel::Root>(buildValue(instruction->getOperand(1)));
                auto cond = buildValue(instruction->getOperand(2));
                ins->addOperand(block2);
                ins->addOperand(cond);
            }
            return;
        }
        case IR::Instruction::Opcode::Load: {
            auto value = buildValue(instruction->getOperand(0));
            ins->addOperand(value);
            return;
        }
        case IR::Instruction::Opcode::Store: {
            auto ptr = buildValue(instruction->getOperand(0));
            auto value = buildValue(instruction->getOperand(1));
            ins->addOperand(ptr);
            ins->addOperand(value);
            return;
        }
        case IR::Instruction::Opcode::Call: {
            auto call = cast<IR::CallInstruction>(instruction);
            auto icall = cast<ISel::Call>(ins);
            auto callee = buildValue(call->getCallee());
            icall->addOperand(callee);
            for(auto& operand : call->getArguments()) {
                auto value = buildValue(operand);
                icall->addOperand(value);
            }
            icall->setResultUsed(call->getUses().size() > 0);
            return;
        }
        case IR::Instruction::Opcode::Switch: {
            IR::SwitchInstruction* switchInstruction = cast<IR::SwitchInstruction>(instruction);
            ISel::Node* condition = buildValue(switchInstruction->getCondition());
            ISel::Root* defaultCase = cast<ISel::Root>(buildValue(switchInstruction->getDefaultCase()));
            ins->addOperand(condition);
            ins->addOperand(defaultCase);
            for(auto casePair : switchInstruction->getCases()) {
                ISel::Root* block = cast<ISel::Root>(buildValue(casePair.second));
                ins->addOperand(buildValue(casePair.first));
                ins->addOperand(block);
            }
            return;
        }
        case IR::Instruction::Opcode::Phi: {
            auto phi = cast<IR::PhiInstruction>(instruction);
            for(size_t i = 0; i < phi->getNumOperands(); i++) {
                auto operand = phi->getOperand(i);
                auto value = buildValue(operand);
                ins->addOperand(value);
            }
            return;
        }
        case IR::Instruction::Opcode::Allocate:
            return;
        default:
            break;
    }
    throw std::runtime_error("Unsupported instruction for " + instruction->getName());
}

void ISelPass::selectPattern(ISel::Node* node) {
    if(m_bestMatch.contains(node)) return;
    m_bestMatch[node] = MatchResult(nullptr, 0, node); // temporary for recursion

    auto& patterns = m_instructionInfo->getPatterns(node->getKind());
    if(patterns.empty()) {
        m_bestMatch.erase(node);
        return;
    }
    
    std::vector<MatchResult> results;
    for(auto& pattern : patterns) {
        if(m_optLevel < pattern.m_minimumOptLevel || !pattern.match(node, m_dataLayout)) continue;
        uint32_t cost = pattern.m_cost;
        if(auto instruction = dyn_cast<ISel::Instruction>(node)) {
            for(size_t i = 0; i < instruction->getOperands().size(); i++) {
                ISel::Node* op = instruction->getOperands().at(i);
                if(std::find(pattern.m_coveredOperands.begin(), pattern.m_coveredOperands.end(), i) != pattern.m_coveredOperands.end()
                        && (op->getRoot() == instruction->getRoot())) {
                    continue;
                }
                selectPattern(op);
                if(m_bestMatch.contains(op))
                    cost += m_bestMatch.at(op).m_cost;
            }
        }
        results.push_back(MatchResult(&pattern, cost, node));
    }

    if(results.empty())
        throw std::runtime_error("No patterns matched for " + std::to_string((uint32_t)node->getKind()));

    auto best = *std::min_element(results.begin(), results.end(), [](const MatchResult& a, const MatchResult& b) {
        return a.m_cost < b.m_cost;
    });

    if(auto instruction = dyn_cast<ISel::Instruction>(best.m_node)) {
        for(size_t covered : best.m_pattern->m_coveredOperands) {
            if(m_bestMatch.contains(instruction->getOperands().at(covered)))
                m_bestMatch.erase(instruction->getOperands().at(covered));
        }
    }

    m_bestMatch[node] = best;
}

MIR::Operand* ISelPass::emitOrGet(ISel::Node* node, MIR::Block* block, bool autoextract) {
    if(autoextract) node = Target::extractOperand(node);

    if(m_nodesToMIROperands.contains(node)) return m_nodesToMIROperands[node];

    if(!m_bestMatch.contains(node))
        throw std::runtime_error("No patterns matched for " + std::to_string((uint32_t)node->getKind()));

    auto& match = m_bestMatch.at(node);
    auto res = match.m_pattern->emit(block, m_dataLayout, m_instructionInfo, node, this, m_context);

    m_nodesToMIROperands[node] = res;
    return res;
}

ISel::Register* ISelPass::makeOrGetRegister(IR::Value* referenceValue, Type* type) {
    if(m_registers.contains(referenceValue)) return m_registers[referenceValue];
    auto reg = std::make_unique<ISel::Register>(referenceValue->getName(), type);
    auto ret = reg.get();
    m_registers[referenceValue] = ret;
    m_inserter.insert(std::move(reg));
    return ret;
}

ISel::ConstantInt* ISelPass::makeOrGetConstInt(int64_t value, Type* type) {
    size_t hash = hashValues(value, type);
    if(m_constantInts.contains(hash)) return m_constantInts[hash];
    auto constant = std::make_unique<ISel::ConstantInt>(value, type);
    auto ret = constant.get();
    m_constantInts[hash] = ret;
    m_inserter.insert(std::move(constant));
    return ret;
}

ISel::ConstantFloat* ISelPass::makeOrGetConstFloat(double value, Type* type) {
    size_t hash = hashValues(value, type);
    if(m_constantFloats.contains(hash)) return m_constantFloats[hash];
    auto constant = std::make_unique<ISel::ConstantFloat>(value, type);
    auto ret = constant.get();
    m_constantFloats[hash] = ret;
    m_inserter.insert(std::move(constant));
    return ret;
}

ISel::FrameIndex* ISelPass::makeOrGetFrameIndex(uint32_t slot, Type* type) {
    size_t hash = hashValues(slot, type);
    if(m_frameIndices.contains(hash)) return m_frameIndices[hash];
    auto frameIndex = std::make_unique<ISel::FrameIndex>(slot, type);
    auto ret = frameIndex.get();
    m_frameIndices[hash] = ret;
    m_inserter.insert(std::move(frameIndex));
    return ret;
}
    
}