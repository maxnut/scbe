#pragma once

#include "data_layout.hpp"
#include "diagnostic_emitter.hpp"
#include "pass.hpp"

namespace scbe::IR {

class Block;

class Verifier : public FunctionPass {
public:
    Verifier(DiagnosticEmitter* emitter, DataLayout* layout) : m_diagnosticEmitter(emitter), m_layout(layout) {}

    bool run(Function* functon);

private:
    void verify(Block* block);
    void verify(Instruction* instruction);

private:
    DiagnosticEmitter* m_diagnosticEmitter = nullptr;
    DataLayout* m_layout = nullptr;
};

}