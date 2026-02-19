#pragma once

#include "IR/instruction.hpp"

#include "type.hpp"

#include <ostream>

namespace scbe {
    class Unit;
}

namespace scbe::IR {

class Block;

class Printer {
public:
    Printer(std::ostream& output) : m_output(output) {}

    virtual void print(Unit& unit) = 0;
    virtual void print(const Function* function) = 0;
    virtual void print(const Block* block) = 0;
    virtual void print(const Instruction* instruction) = 0;
    virtual void print(const Value* value) = 0;
    virtual void print(const Type* type) = 0;

    void printIndentation();

protected:
    std::ostream& m_output;
    uint32_t m_indent = 0;
};

class HumanPrinter : public Printer {
public:
    HumanPrinter(std::ostream& output) : Printer(output) {}

    void print(Unit& unit) override;
    void print(const Function* function) override;
    void print(const Block* block) override;
    void print(const Instruction* instruction) override;
    void print(const Value* value) override;
    void print(const Type* type) override;

private:
    USet<const StructType*> m_deferPrintStruct;
};

}