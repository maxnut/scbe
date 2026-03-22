#pragma once

#include "function.hpp"
#include "intrinsic_name.hpp"

namespace scbe {
class Context;
}

namespace scbe::IR {

class IntrinsicFunction : public Function {
public:

    IntrinsicName getIntrinsicName() const { return m_intrinsicName; }

private:
    static std::unique_ptr<IntrinsicFunction> get(IntrinsicName name, Ref<Context> ctx);

    IntrinsicFunction(const std::string& name, FunctionType* type, IntrinsicName intrinsicName) : Function(name, type, Linkage::Internal), m_intrinsicName(intrinsicName) { m_isIntrinsic = true; }

private:
    IntrinsicName m_intrinsicName;

friend class scbe::Unit;
};

}