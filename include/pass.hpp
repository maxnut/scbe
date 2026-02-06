#pragma once

#include "type_alias.hpp"

namespace scbe {

class Unit;

namespace IR {
class Function;
class Instruction;
};

namespace MIR {
class Function;
class Instruction;
};

class Pass {
public:
    enum class Kind {
        Function,
        MachineFunction,
        Instruction,
        MachineInstruction
    };

    Pass(Kind kind) : m_kind(kind) {}
    virtual ~Pass() = default;

    Kind getKind() const { return m_kind; }

    virtual void init(Unit& unit) {}
    virtual void end(Unit& unit) {}

protected:
    Kind m_kind;
};

class FunctionPass : public Pass {
public:
    FunctionPass() : Pass(Kind::Function) {}

    virtual bool run(IR::Function* function) = 0;
};

class MachineFunctionPass : public Pass {
public:
    MachineFunctionPass() : Pass(Kind::MachineFunction) {}

    virtual bool run(MIR::Function* function) = 0;
};

class InstructionPass : public Pass {
public:
    InstructionPass() : Pass(Kind::Instruction) {}

    virtual bool run(IR::Instruction* instruction) = 0;

    void restart() { m_restart = true; }

private:
    bool m_restart = false;
friend class PassManager;
};

class MachineInstructionPass : public Pass {
public:
    MachineInstructionPass() : Pass(Kind::MachineInstruction) {}

    virtual bool run(MIR::Instruction* instruction) = 0;

    void restart() { m_restart = true; }

private:
    bool m_restart = false;
friend class PassManager;
};

    
};