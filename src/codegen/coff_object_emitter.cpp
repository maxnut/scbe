#include "codegen/coff_object_emitter.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "codegen/fixup.hpp"
#include "coffi/coffi_types.hpp"
#include "unit.hpp"

#include <coffi/coffi.hpp>

using namespace COFFI;

#define ADDR64 1
#define ADDR32NB 3
#define REL32 4
#define SECTION 10
#define SECREL 11

namespace scbe::Codegen {

void COFFObjectEmitter::emitObjectFile(Unit& unit) {
    coffi writer;
    writer.create(COFFI_ARCHITECTURE_PE);
    writer.get_header()->set_machine(IMAGE_FILE_MACHINE_AMD64);

    section* textSec = writer.add_section(".text");
    textSec->set_flags(IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_ALIGN_8BYTES);

    section* dataSec = writer.add_section(".data");
    dataSec->set_flags(IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_ALIGN_4BYTES);
    dataSec->set_data((const char*)m_dataBytes.data(), m_dataBytes.size());

    section* bssSec = writer.add_section(".bss");
    bssSec->set_flags(IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE |
                       IMAGE_SCN_CNT_UNINITIALIZED_DATA |
                       IMAGE_SCN_ALIGN_4BYTES);

    section* vSec = writer.add_section(".rdata$zzz");
    vSec->set_flags(IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA |
                     IMAGE_SCN_ALIGN_4BYTES);

    UMap<std::string, symbol*> symbols;

    for(const auto& [name, dataEntry] : m_dataLocTable) {
        symbol* sym = writer.add_symbol(name);
        sym->set_section_number(dataSec->get_index() + 1);
        sym->set_value(dataEntry.m_loc);
        sym->set_type(IMAGE_SYM_TYPE_NULL); // maybe TODO map type to this
        sym->set_storage_class(dataEntry.m_globalAddress->getValue()->getLinkage() == IR::Linkage::External ? IMAGE_SYM_CLASS_EXTERNAL : IMAGE_SYM_CLASS_STATIC);
        auxiliary_symbol_record a{};
        sym->get_auxiliary_symbols().push_back(a);
        symbols.insert({name, sym});
    }

    for(const auto& ext : unit.getExternals()) {
        symbol* sym = writer.add_symbol(ext.second->getName());
        sym->set_section_number(IMAGE_SYM_UNDEFINED);
        sym->set_value(0);
        sym->set_type(IMAGE_SYM_TYPE_NULL); // maybe TODO map type to this
        sym->set_storage_class(IMAGE_SYM_CLASS_EXTERNAL);
        sym->set_aux_symbols_number(1);
        auxiliary_symbol_record a{};
        sym->get_auxiliary_symbols().push_back(a);
        symbols.insert({ext.second->getName(), sym});
    }

    for(auto& function : unit.getFunctions()) {
        if(!function->hasBody()) continue;
        symbol* sym = writer.add_symbol(function->getName());
        sym->set_section_number(textSec->get_index() + 1);
        sym->set_value(m_codeLocTable.at(function->getName()));
        sym->set_type(IMAGE_SYM_TYPE_FUNCTION);
        sym->set_storage_class(IMAGE_SYM_CLASS_EXTERNAL);
        sym->set_aux_symbols_number(1);
        auxiliary_symbol_record a2{
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
        sym->get_auxiliary_symbols().push_back(a2);
        symbols.insert({function->getName(), sym});

        for(auto& block : function->getBlocks()) {
            symbol* sym = writer.add_symbol(block->getName());
            sym->set_section_number(textSec->get_index() + 1);
            sym->set_value(m_codeLocTable.at(block->getName()));
            sym->set_type(IMAGE_SYM_TYPE_NOT_FUNCTION);
            sym->set_storage_class(IMAGE_SYM_CLASS_EXTERNAL);
            sym->set_aux_symbols_number(1);
            auxiliary_symbol_record a2{
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
            sym->get_auxiliary_symbols().push_back(a2);
            symbols.insert({block->getName(), sym});
        }
    }

    uint16_t textRelocations = 0, dataRelocations = 0;

    for(auto& fixup : m_fixups) {
        if(fixup.getSection() == Fixup::Data || (fixup.getSection() == Fixup::Text && !m_codeLocTable.contains(fixup.getSymbol()))) {
            if(!symbols.contains(fixup.getSymbol())) {
                throw std::runtime_error("Could not find symbol " + fixup.getSymbol());
            }
            rel_entry_generic rela;
            rela.virtual_address = fixup.getLocation();
            rela.symbol_table_index = symbols.at(fixup.getSymbol())->get_index();
            rela.reserved = 0;
            if(fixup.getSection() == Fixup::Text) {
                rela.type = REL32;
                int32_t rel = /* target offset */ - (fixup.getLocation() + fixup.getInstructionSize() + fixup.getAddend());
                for (size_t i = 0; i < 4; i++) {
                    m_codeBytes[fixup.getLocation() + i] = rel & 0xFF;
                    rel >>= 8;
                }

                textSec->add_relocation_entry(&rela);
                textRelocations++;
            }
            else if(fixup.getSection() == Fixup::Data) {
                int32_t rel = fixup.getAddend();
                for (size_t i = 0; i < 4; i++) {
                    m_codeBytes[fixup.getLocation() + i] = rel & 0xFF;
                    rel >>= 8;
                }
                rela.type = ADDR64;
                dataSec->add_relocation_entry(&rela);
                dataRelocations++;
            }
            continue;
        }
        size_t loc = m_codeLocTable.at(fixup.getSymbol());
        size_t instructionLocation = fixup.getLocation() - (fixup.getInstructionSize() - 4);
        int32_t off = loc - (instructionLocation + fixup.getInstructionSize());
        for(size_t i = 0; i < 4; i++) {
            m_codeBytes[fixup.getLocation() + i] = off & 0xFF;
            off >>= 8;
        }
    }

    textSec->set_data((const char*)m_codeBytes.data(), m_codeBytes.size());

    writer.add_symbol(".file");

    symbol* textSym = writer.add_symbol(".text");
    textSym->set_type(IMAGE_SYM_TYPE_NOT_FUNCTION);
    textSym->set_storage_class(IMAGE_SYM_CLASS_STATIC);
    textSym->set_section_number(textSec->get_index() + 1);
    textSym->set_aux_symbols_number(1);
    auxiliary_symbol_record_5 a3{
        textSec->get_data_size(), textRelocations, 0, 0, 0, 0, {0, 0, 0}};
    textSym->get_auxiliary_symbols().push_back(
        *reinterpret_cast<auxiliary_symbol_record*>(&a3));

    symbol* dataSym = writer.add_symbol(".data");
    dataSym->set_type(IMAGE_SYM_TYPE_NOT_FUNCTION);
    dataSym->set_storage_class(IMAGE_SYM_CLASS_STATIC);
    dataSym->set_section_number(dataSec->get_index() + 1);
    dataSym->set_aux_symbols_number(1);
    auxiliary_symbol_record_5 a4{
        dataSec->get_data_size(), dataRelocations, 0, 0, 0, 0, {0, 0, 0}};
    dataSym->get_auxiliary_symbols().push_back(
        *reinterpret_cast<auxiliary_symbol_record*>(&a4));

    symbol* bssSym = writer.add_symbol(".bss");
    bssSym->set_type(IMAGE_SYM_TYPE_NOT_FUNCTION);
    bssSym->set_storage_class(IMAGE_SYM_CLASS_STATIC);
    bssSym->set_section_number(bssSec->get_index() + 1);
    bssSym->set_aux_symbols_number(1);
    auxiliary_symbol_record_5 a5{
        bssSec->get_data_size(), 0, 0, 0, 0, 0, {0, 0, 0}};
    bssSym->get_auxiliary_symbols().push_back(
        *reinterpret_cast<auxiliary_symbol_record*>(&a5));

    symbol* rdataSym = writer.add_symbol(".rdata$zzz");
    rdataSym->set_type(IMAGE_SYM_TYPE_NOT_FUNCTION);
    rdataSym->set_storage_class(IMAGE_SYM_CLASS_STATIC);
    rdataSym->set_section_number(vSec->get_index() + 1);
    rdataSym->set_aux_symbols_number(1);
    auxiliary_symbol_record_5 a6{
        vSec->get_data_size(), 0, 0, 0, 0, 0, {0, 0, 0}};
    rdataSym->get_auxiliary_symbols().push_back(
        *reinterpret_cast<auxiliary_symbol_record*>(&a6));

    writer.save(m_output);
}

}