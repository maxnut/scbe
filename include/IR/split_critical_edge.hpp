#pragma once

#include "pass.hpp"

namespace scbe::IR {

class SplitCriticalEdge : public FunctionPass {
public:
    SplitCriticalEdge(Ref<Context> ctx) : m_ctx(ctx) {}

    bool run(Function* function) override;

private:
    Ref<Context> m_ctx = nullptr;
};

}