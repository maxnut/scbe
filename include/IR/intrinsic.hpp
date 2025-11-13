#pragma once

#include "function.hpp"

namespace scbe {
class Context;
}

namespace scbe::IR {

class IntrinsicFunction : public Function {
public:
    enum Name {
        Memcpy
    };

    static std::unique_ptr<IntrinsicFunction> get(Name name, Ref<Context> ctx);

    Name getIntrinsicName() const { return m_intrinsicName; }

private:
    IntrinsicFunction(const std::string& name, FunctionType* type, Name intrinsicName) : Function(name, type, Linkage::Internal), m_intrinsicName(intrinsicName) { m_isIntrinsic = true; }

private:
    Name m_intrinsicName;
};

}