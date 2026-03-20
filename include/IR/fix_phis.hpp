#pragma once

#include "pass.hpp"

namespace scbe::IR {

class FixPhis : public InstructionPass {
public:
    bool run(Instruction* instruction) override;
};

}