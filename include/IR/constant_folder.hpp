#pragma once

#include "pass.hpp"
#include "IR/folder.hpp"

namespace scbe {
class Context;
}

namespace scbe::IR {

class ConstantFolder : public InstructionPass {
public:
    ConstantFolder(Ref<Context> context) : InstructionPass(), m_folder(context), m_context(context) {}

    bool run(IR::Instruction* instruction) override;

private:
    IR::Folder m_folder;
    Ref<Context> m_context;
};

}