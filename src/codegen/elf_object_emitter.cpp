#include "codegen/elf_object_emitter.hpp"
#include "IR/function.hpp"
#include "IR/block.hpp"
#include "codegen/fixup.hpp"
#include "elfio/elf_types.hpp"
#include "unit.hpp"

#include <elfio/elfio.hpp>

using namespace ELFIO;

namespace scbe::Codegen {

void ELFObjectEmitter::emitObjectFile(Unit& unit) {
    elfio writer;
    writer.create(ELFCLASS64, ELFDATA2LSB);
    writer.set_type(ET_REL);
    writer.set_machine(EM_X86_64); // TODO map machine
    writer.set_os_abi(ELFOSABI_LINUX); // TODO map os to this

    section* textSection = writer.sections.add(".text");
    textSection->set_type( SHT_PROGBITS );
    textSection->set_flags( SHF_ALLOC | SHF_EXECINSTR );
    textSection->set_addr_align( 0x10 );

    section* data_sec = writer.sections.add(".data");
    data_sec->set_type(SHT_PROGBITS);
    data_sec->set_flags(SHF_ALLOC);
    data_sec->set_addr_align(1);
    data_sec->set_data((const char*)m_dataBytes.data(), m_dataBytes.size());

    section* str_sec = writer.sections.add( ".strtab" );
    str_sec->set_type( SHT_STRTAB );

    section* sym_sec = writer.sections.add( ".symtab" );
    sym_sec->set_type( SHT_SYMTAB );
    sym_sec->set_info( 1 );
    sym_sec->set_addr_align( 0x4 );
    sym_sec->set_entry_size( writer.get_default_entry_size( SHT_SYMTAB ) );
    sym_sec->set_link( str_sec->get_index() );

    symbol_section_accessor syma( writer, sym_sec );
    string_section_accessor stra( str_sec );

    UMap<std::string, Elf32_Word> symbols;
    UMap<std::string, Elf_Word> symbolLocations;

    for(const auto& [name, dataEntry] : m_dataLocTable) {
        Elf32_Word index = stra.add_string(name);
        symbols.insert({name, index});
        Elf_Word sym = syma.add_symbol(
        index, dataEntry.m_loc, 0,
            dataEntry.m_globalAddress->getValue()->getLinkage() == IR::Linkage::External ? STB_GLOBAL : STB_LOCAL,
            STT_OBJECT, 0, data_sec->get_index() );
        symbolLocations.insert({name, sym});
    }

    for(const auto& ext : unit.getExternals()) {
        Elf32_Word index = stra.add_string(ext.second->getName());
        symbols.insert({ext.second->getName(), index});
        Elf_Word sym = syma.add_symbol(index, 0, 0, STB_GLOBAL, STT_NOTYPE, 0, SHN_UNDEF);
        symbolLocations.insert({ext.second->getName(), sym});
    }

    for(auto& function : unit.getFunctions()) {
        if(!function->hasBody()) continue;
        Elf32_Word index = stra.add_string(function->getName());
        symbols.insert({function->getName(), index});
        Elf_Word sym = syma.add_symbol(
        index, m_codeLocTable.at(function->getName()), 0,
            function->getLinkage() == IR::Linkage::External ? STB_GLOBAL : STB_LOCAL,
            STT_FUNC, 0, textSection->get_index() );
        symbolLocations.insert({function->getName(), sym});

        for(auto& block : function->getBlocks()) {
            Elf32_Word index = stra.add_string(block->getName());
            symbols.insert({block->getName(), index});
            Elf_Word sym = syma.add_symbol(
            index, m_codeLocTable.at(block->getName()), 0,
                function->getLinkage() == IR::Linkage::External ? STB_GLOBAL : STB_LOCAL,
                STT_SECTION, 0, textSection->get_index() );
            symbolLocations.insert({block->getName(), sym});
        }
    }
        
    section* relTextSec = writer.sections.add( ".rela.text" );
    relTextSec->set_type( SHT_RELA );
    relTextSec->set_info( textSection->get_index() );
    relTextSec->set_addr_align( 8 );
    relTextSec->set_entry_size( writer.get_default_entry_size( SHT_RELA ) );
    relTextSec->set_link( sym_sec->get_index() );
    relocation_section_accessor relaText( writer, relTextSec );

    section* relDataSec = writer.sections.add( ".rela.data" );
    relDataSec->set_type( SHT_RELA );
    relDataSec->set_info( data_sec->get_index() );
    relDataSec->set_addr_align( 8 );
    relDataSec->set_entry_size( writer.get_default_entry_size( SHT_RELA ) );
    relDataSec->set_link( sym_sec->get_index() );
    relocation_section_accessor relaData( writer, relDataSec );

    for(auto& fixup : m_fixups) {
        if(fixup.getSection() == Fixup::Data || (fixup.getSection() == Fixup::Text && !m_codeLocTable.contains(fixup.getSymbol()))) {
            if(!symbolLocations.contains(fixup.getSymbol())) {
                throw std::runtime_error("Could not find symbol " + fixup.getSymbol());
            }
            if(fixup.getSection() == Fixup::Text) {
                uint32_t type = R_X86_64_PC32;
                if(unit.getExternals().contains(fixup.getSymbol())) type = R_X86_64_PLT32;
                relaText.add_entry( fixup.getLocation(), symbolLocations.at(fixup.getSymbol()),
                        (unsigned char)(type), fixup.getAddend()-4 ); // TODO pick these based on spec
            }
            else if(fixup.getSection() == Fixup::Data) {
                relaData.add_entry( fixup.getLocation(), symbolLocations.at(fixup.getSymbol()),
                        (unsigned char)(R_X86_64_64), fixup.getAddend() );
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

    textSection->set_data( (const char*)m_codeBytes.data(), m_codeBytes.size() );

    // section* note_sec = writer.sections.add( ".note" );
    // note_sec->set_type( SHT_NOTE );

    // note_section_accessor note_writer( writer, note_sec );
    // note_writer.add_note( 0x01, "Created by ELFIO", 0, 0 );
    // char descr[6] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36 };
    // note_writer.add_note( 0x01, "Never easier!", descr, sizeof( descr ) );

    // remove annoying note
    section* gs = writer.sections.add(".note.GNU-stack");
    gs->set_type(SHT_PROGBITS);
    gs->set_flags(0);
    gs->set_addr_align(1);

    syma.arrange_local_symbols( [&]( Elf_Xword first, Elf_Xword second ) {
        relaData.swap_symbols( first, second );
        relaText.swap_symbols( first, second );
    } );

    writer.save(m_output);
}

}