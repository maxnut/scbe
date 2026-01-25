#pragma once

#include "pass.hpp"
#include "target/target_specification.hpp"

namespace scbe {
class Context;
}

namespace scbe::Target::x64 {

class x64Legalizer : public InstructionPass {
public:
    x64Legalizer(Ref<Context> context, TargetSpecification spec) : m_context(context), m_spec(spec) {}

    virtual void init(Unit& unit) override;
    virtual bool run(IR::Instruction* instruction) override;

private:
    Ref<Context> m_context = nullptr;;
    TargetSpecification m_spec;
};

}