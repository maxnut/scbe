#pragma once

#include <string>

namespace scbe::Codegen {

class Fixup {
public:
    enum Section {
        Text = 0,
        Data = 1
    };

    Fixup(std::string symbol, size_t location, size_t instructionSize, Section section, int64_t addend = 0)
        : m_symbol(symbol), m_location(location), m_instructionSize(instructionSize), m_section(section), m_addend(addend) {}

    const std::string& getSymbol() const { return m_symbol; }
    size_t getLocation() const { return m_location; }
    size_t getInstructionSize() const { return m_instructionSize; }
    Section getSection() const { return m_section; }
    int64_t getAddend() const { return m_addend; }

private:
    std::string m_symbol;
    size_t m_location;
    size_t m_instructionSize;
    Section m_section;
    bool m_gotpcrel = false;
    int64_t m_addend = 0;
};

}