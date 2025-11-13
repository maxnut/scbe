#include "IR/intrinsic.hpp"
#include "context.hpp"

namespace scbe::IR {

std::unique_ptr<IntrinsicFunction> IntrinsicFunction::get(Name name, Ref<Context> ctx) {
    switch (name) {
        case Name::Memcpy:
            return std::unique_ptr<IntrinsicFunction>(new IntrinsicFunction(
                ".intrinsic.memcpy",
                ctx->makeFunctionType({ctx->makePointerType(ctx->getI8Type()), ctx->makePointerType(ctx->getI8Type()), ctx->getI64Type()}, ctx->getVoidType()),
                Name::Memcpy
            ));
    }
    return nullptr;
}

}