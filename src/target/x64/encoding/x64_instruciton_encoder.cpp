#include "cast.hpp"
#include "codegen/fixup.hpp"
#include "target/x64/encoding/x64_instruction_encoder.hpp"
#include "target/x64/encoding/x64_mapping.hpp"
#include <optional>

namespace scbe::Target::x64 {

std::optional<Codegen::Fixup> x64InstructionEncoder::encode(MIR::Instruction* instruction, UMap<std::string, size_t> symbols, std::vector<uint8_t>& bytes) {
    std::optional<Codegen::Fixup> fixup = std::nullopt;
    
    const InstructionDescriptor& descriptor = m_instructionInfo->getInstructionDescriptor(instruction->getOpcode());
    if(descriptor.getName().empty()) throw std::runtime_error("Instruction descriptor not found");
    if(instruction->getOpcode() > Mapping::s_instructionMapping.size()) throw std::runtime_error("Instruction encoding not found");
    InstructionEncoding encoding = Mapping::s_instructionMapping[instruction->getOpcode()];
    if(encoding.m_numBytes == 0) throw std::runtime_error("Instruction encoding not found");

    bool rexW = descriptor.getSize() == 8 && !encoding.m_default64Bit;
    bool rexR = false, rexX = false, rexB = false;

    size_t instructionPos = bytes.size();

    if(descriptor.getSize() == 2) bytes.push_back(0x66);

    if(encoding.m_operandType == InstructionEncoding::Embedded) {
        uint8_t reg = encodeRegister(cast<MIR::Register>(instruction->getOperands().at(0))->getId());
        rexB = isExtendedRegister(reg);

        if(rexW || rexB)
            bytes.push_back(encodeREX(rexW, rexR, rexX, rexB));

        for(size_t i = 0; i < encoding.m_numBytes; i++) {
            uint8_t byte = encoding.m_bytes[i];
            bytes.push_back(i == 0 ? byte + (reg & 0x7) : byte);
        }

        if(encoding.m_immediate) {
            int64_t imm = cast<MIR::ImmediateInt>(instruction->getOperands().at(1))->getValue();
            for(size_t i = 0; i < encoding.m_immediateSize; i++) {
                bytes.push_back(imm & 0xFF);
                imm >>= 8;
            }
        }

        return fixup;
    }
    else if(encoding.m_operandType == InstructionEncoding::Normal) {
        uint8_t mod = 0, reg = 0, rm = 0; 
        uint8_t disp8 = 0;
        uint32_t disp32 = 0;
        uint8_t base = 0, index = 0, scale = 0;
        bool hasDisp = false;
        bool hasSIB = false;
        auto& ops = instruction->getOperands();
        MIR::ImmediateInt* optImmediate = nullptr;

        if(descriptor.mayLoad() || descriptor.mayStore()) {
            bool isLoad = descriptor.mayLoad();
            
            MIR::Symbol* symbolOpt = cast<MIR::Symbol>(ops.at(4 + isLoad));
            MIR::Register* mirbase = cast<MIR::Register>(ops.at(0 + isLoad));
            int64_t miroffset = cast<MIR::ImmediateInt>(ops.at(3 + isLoad))->getValue();
            MIR::Register* mirindex = cast<MIR::Register>(ops.at(2 + isLoad));
            int64_t mirscale = cast<MIR::ImmediateInt>(ops.at(1 + isLoad))->getValue();
            hasDisp = true;

            if(encoding.m_immediate) {
                optImmediate = cast<MIR::ImmediateInt>(ops.at(5)); // can only have immediates on stores
            }
            else {
                reg = encodeRegister(cast<MIR::Register>(ops.at(isLoad ? 0 : 5))->getId());
                rexR = isExtendedRegister(reg);
            }

            if(symbolOpt) {
                mod = 0b00;
                rm = 0b101;
                uint8_t rexByte = rexW || rexR || rexX || rexB ? 1 : 0;
                if(!symbols.contains(symbolOpt->getName())) {
                    fixup = Codegen::Fixup(symbolOpt->getName(), bytes.size() + rexByte + encoding.m_numBytes + 1, rexByte + encoding.m_numBytes + 1 + 4, Codegen::Fixup::Text, isGotpcrel(symbolOpt));
                    miroffset = 0;
                }
                else {
                    miroffset = symbols.at(symbolOpt->getName());
                }
            }
            else if(mirbase && !mirindex) {
                if((miroffset == 0 && (mirbase->getId() != RBP && mirbase->getId() != R13)) || mirbase->getId() == RIP) mod = 0; // TODO check
                else if(miroffset >= -128 && miroffset <= 127) mod = 0b01;
                else mod = 0b10;

                if(mirbase->getId() == RSP || mirbase->getId() == R12) { // TODO check
                    rm = 0b100;
                    hasSIB = true;
                    index = 0b100;
                    scale = 0;
                    base = encodeRegister(mirbase->getId());
                    rexB = isExtendedRegister(base);
                }
                else {
                    rm = mirbase->getId() == RIP ? 0b101 : encodeRegister(mirbase->getId());
                    rexB = isExtendedRegister(rm);
                }
            }
            else if(mirbase && mirindex) {
                if(miroffset == 0 && (mirbase->getId() != RBP && mirbase->getId() != R13)) mod = 0; // TODO check
                else if(miroffset >= -128 && miroffset <= 127) mod = 0b01;
                else mod = 0b10;

                rm = 0b100;
                hasSIB = true;
                switch(mirscale) {
                    default: throw std::runtime_error("Invalid scale");
                    case 1: scale = 0; break;
                    case 2: scale = 1; break;
                    case 4: scale = 2; break;
                    case 8: scale = 3; break;
                }
                index = encodeRegister(mirindex->getId());
                base = encodeRegister(mirbase->getId());
                rexB = isExtendedRegister(base);
                rexX = isExtendedRegister(index);
            }
            else throw std::runtime_error("Invalid operand");

            if(miroffset >= -128 && miroffset <= 127) disp8 = miroffset;
            else disp32 = miroffset;
        }
        else {
            if(ops.size() > 1) {
                if(ops.at(0)->isRegister() && ops.at(1)->isRegister()) {
                    mod = 0b11;
                    reg = encodeRegister(cast<MIR::Register>(ops.at(encoding.m_regIdx))->getId());
                    rm = encodeRegister(cast<MIR::Register>(ops.at(encoding.m_rmIdx))->getId());
                    rexR = isExtendedRegister(reg);
                    rexB = isExtendedRegister(rm);
                }
                else if(ops.at(0)->isRegister() && ops.at(1)->isImmediateInt()) {
                    mod = 0b11;
                    if(encoding.m_rmIdx == 0) {
                        rm = encodeRegister(cast<MIR::Register>(ops.at(0))->getId());
                        rexB = isExtendedRegister(rm);
                    }
                    else if(encoding.m_regIdx == 0) {
                        reg = encodeRegister(cast<MIR::Register>(ops.at(0))->getId());
                        rexR = isExtendedRegister(reg);
                    }
                    optImmediate = cast<MIR::ImmediateInt>(ops.at(1));
                }

                if(ops.size() > 2 && ops.at(2)->isImmediateInt()) {
                    optImmediate = cast<MIR::ImmediateInt>(ops.at(2));
                }
            }
            else {
                if(ops.at(0)->isRegister()) {
                    mod = 0b11;
                    if(encoding.m_regIdx == 0) {
                        reg = encodeRegister(cast<MIR::Register>(ops.at(0))->getId());
                        rexR = isExtendedRegister(reg);
                    }
                    else if(encoding.m_rmIdx == 0) {
                        rm = encodeRegister(cast<MIR::Register>(ops.at(0))->getId());
                        rexB = isExtendedRegister(rm);
                    }
                }
                else if(ops.at(0)->isImmediateInt()) {
                    mod = 0b11;
                    optImmediate = cast<MIR::ImmediateInt>(ops.at(0));
                }
            }
        }

        for(size_t i = 0; i < encoding.m_numBytes; i++) {
            if(encoding.m_rexPosition == i && (rexW || rexR || rexX || rexB))
                bytes.push_back(encodeREX(rexW, rexR, rexX, rexB));

            bytes.push_back(encoding.m_bytes[i]);
        }

        bytes.push_back(encodeModRM(mod, encoding.m_instructionVariant ? *encoding.m_instructionVariant : (reg & 0x7), (rm == 0b100 || rm == 0b101 ? rm : rm & 0x7)));

        if(hasDisp) {
            if(mod == 0b01)
                bytes.push_back(disp8);
            else if(mod == 0b10 || mod == 0 && rm == 0b101 /* RIP */) {
                for(size_t i = 0; i < 4; i++) {
                    bytes.push_back(disp32 & 0xFF);
                    disp32 >>= 8;
                }
            }
        }
        if(encoding.m_immediate) {
            int64_t imm = optImmediate->getValue();
            for(size_t i = 0; i < encoding.m_immediateSize; i++) {
                bytes.push_back(imm & 0xFF);
                imm >>= 8;
            }
        }
        if(hasSIB)
            bytes.push_back(encodeSIB(scale, index, base));
    }
    else if(encoding.m_operandType == InstructionEncoding::Symbol) {
        uint32_t instructionAddress = bytes.size();

        for(size_t i = 0; i < encoding.m_numBytes; i++)
            bytes.push_back(encoding.m_bytes[i]);
        MIR::Symbol* symbol = cast<MIR::Symbol>(instruction->getOperands().at(0));
        size_t instructionSize = encoding.m_numBytes + 4;
        uint32_t loc = 0;

        if(!symbols.contains(symbol->getName()))
            fixup = Codegen::Fixup(symbol->getName(), bytes.size(), instructionSize, Codegen::Fixup::Text, isGotpcrel(symbol));
        else
            loc = symbols.at(symbol->getName());

        int32_t rel32 = int32_t(loc) - int32_t(instructionAddress + 5);

        for(size_t i = 0; i < 4; i++) {
            bytes.push_back(rel32 & 0xFF);
            rel32 >>= 8;
        }
    }
    else {
        for(size_t i = 0; i < encoding.m_numBytes; i++)
            bytes.push_back(encoding.m_bytes[i]);
    }

    return fixup;
}

uint8_t x64InstructionEncoder::encodeREX(uint8_t w, uint8_t r, uint8_t x, uint8_t b) const {
    return 0b01000000 | (w << 3) | (r << 2) | (x << 1) | b;
}

uint8_t x64InstructionEncoder::encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm) const {
    return (mod << 6) | (reg << 3) | rm;
}

uint8_t x64InstructionEncoder::encodeSIB(uint8_t scale, uint8_t index, uint8_t base) const {
    return (scale << 6) | (index << 3) | base;
}

uint8_t x64InstructionEncoder::encodeRegister(uint32_t reg) const {
    assert(reg != RIP && "Cannot encode RIP");
    return Mapping::s_registerMapping[reg];
}

bool x64InstructionEncoder::isExtendedRegister(uint8_t reg) const {
    return reg >= 8;
}

bool x64InstructionEncoder::isGotpcrel(MIR::Symbol* symbol) {
    if(m_spec.getOS() == OS::Windows) return false; // windows does not support gotpcrel
    if(symbol->isExternalSymbol()) return true;

    if(!symbol->isGlobalAddress()) return false;
    MIR::GlobalAddress* add = cast<MIR::GlobalAddress>(symbol);
    if(add->getValue()->isGlobalVariable()) {
        IR::GlobalVariable* var = cast<IR::GlobalVariable>(add->getValue());
        return var->getValue() == nullptr;
    }
    else if(add->getValue()->isFunction()) return true;
    return false;
}


}