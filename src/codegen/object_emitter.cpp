#include "codegen/object_emitter.hpp"
#include "IR/value.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "cast.hpp"
#include "type.hpp"
#include "unit.hpp"
#include "MIR/function.hpp"

namespace scbe::Codegen {

bool ObjectEmitter::run(MIR::Function* function) {
    m_codeLocTable.insert({function->getName(), m_codeBytes.size()});

    for(auto& block : function->getBlocks()) {
        m_codeLocTable.insert({block->getName(), m_codeBytes.size()});
        
        for(size_t i = 0; i < block->getInstructions().size(); i++) {
            MIR::Instruction* instruction = block->getInstructions()[i].get();
            if(std::optional<Fixup> fixup = m_encoder->encode(instruction, m_codeLocTable, m_codeBytes)) {
                m_fixups.push_back(fixup.value());
            }
        }
    }
    return false;
}

void ObjectEmitter::end(Unit& unit) {
    emitObjectFile(unit);
}

void ObjectEmitter::init(Unit& unit) {
    m_codeBytes.clear();
    m_dataBytes.clear();
    m_fixups.clear();
    m_codeLocTable.clear();
    m_dataLocTable.clear();

    for(auto& globalVariable : unit.getGlobals()) {
        IR::Constant* value = globalVariable->getValue();
        auto l = globalVariable->getLinkage();

        if(!value) continue;
        m_dataLocTable[globalVariable->getName()] = { m_dataBytes.size(), globalVariable->getMachineGlobalAddress(unit) };
        encodeConstant(value, unit.getDataLayout());
    }
}

void ObjectEmitter::encodeConstant(IR::Constant* constant, DataLayout* layout) {
    switch (constant->getKind()) {
        case IR::Value::ValueKind::ConstantInt: {
            IR::ConstantInt* constantInt = cast<IR::ConstantInt>(constant);
            IntegerType* integerType = (IntegerType*)(constantInt->getType());
            int64_t value = constantInt->getValue();
            m_dataBytes.insert(m_dataBytes.end(), (uint8_t*)&value, (uint8_t*)&value + std::max(1, integerType->getBits() / 8));
            break;
        }
        case IR::Value::ValueKind::ConstantString: {
            IR::ConstantString* constantString = cast<IR::ConstantString>(constant);
            m_dataBytes.insert(m_dataBytes.end(), constantString->getValue().begin(), constantString->getValue().end());
            m_dataBytes.push_back(0);
            break;
        }
        case IR::Value::ValueKind::ConstantFloat: {
            IR::ConstantFloat* constantFloat = cast<IR::ConstantFloat>(constant);
            if(cast<FloatType>(constantFloat->getType())->getBits() == 32) {
                float value = (float)constantFloat->getValue();
                m_dataBytes.insert(m_dataBytes.end(), (uint8_t*)&value, (uint8_t*)&value + sizeof(float));
            }
            else {
                double value = constantFloat->getValue();
                m_dataBytes.insert(m_dataBytes.end(), (uint8_t*)&value, (uint8_t*)&value + sizeof(double));
            }
            break;
        }
        case IR::Value::ValueKind::ConstantStruct: {
            IR::ConstantStruct* constantStruct = cast<IR::ConstantStruct>(constant);
            for(auto& value : constantStruct->getValues())
                encodeConstant(value, layout);
            break;
        }
        case IR::Value::ValueKind::ConstantArray: {
            IR::ConstantArray* constantArray = cast<IR::ConstantArray>(constant);
            for(auto& value : constantArray->getValues())
                encodeConstant(value, layout);
            break;
        }
        case IR::Value::ValueKind::Function: {
            IR::Function* function = cast<IR::Function>(constant);
            size_t loc = 0;
            m_fixups.push_back(Fixup(function->getName(), m_dataBytes.size(), 0, Fixup::Section::Data, false));

            m_dataBytes.insert(m_dataBytes.end(), (uint8_t*)&loc, (uint8_t*)&loc + sizeof(size_t));
            break;
        }
        case IR::Value::ValueKind::Block: {
            IR::Block* block = cast<IR::Block>(constant);
            size_t loc = 0;
            m_fixups.push_back(Fixup(block->getName(), m_dataBytes.size(), 0, Fixup::Section::Data, false));

            m_dataBytes.insert(m_dataBytes.end(), (uint8_t*)&loc, (uint8_t*)&loc + sizeof(size_t));
            break;
        }
        case IR::Value::ValueKind::NullValue: {
            int64_t value = 0;
            m_dataBytes.insert(m_dataBytes.end(), (uint8_t*)&value, (uint8_t*)&value + std::max(1UL, layout->getPointerSize()));
            break;
        }
        default:
            throw std::runtime_error("Not implemented");
    }
}

}