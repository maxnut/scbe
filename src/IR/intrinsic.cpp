#include "IR/intrinsic.hpp"
#include "context.hpp"

namespace scbe::IR {

std::unique_ptr<IntrinsicFunction> IntrinsicFunction::get(IntrinsicName name, Ref<Context> ctx) {
    Type* vptr = ctx->makePointerType(ctx->getVoidType());
    switch (name) {
        case IntrinsicName::Memcpy:
            return std::unique_ptr<IntrinsicFunction>(new IntrinsicFunction(
                ".intrinsic.memcpy",
                ctx->makeFunctionType({vptr, vptr, ctx->getI64Type()}, ctx->getVoidType()),
                name
            ));
        case IntrinsicName::VaStart:
            return std::unique_ptr<IntrinsicFunction>(new IntrinsicFunction(
                ".intrinsic.va_start",
                ctx->makeFunctionType({vptr}, ctx->getVoidType()),
                name
            ));
        case IntrinsicName::VaEnd:
            return std::unique_ptr<IntrinsicFunction>(new IntrinsicFunction(
                ".intrinsic.va_end",
                ctx->makeFunctionType({vptr}, ctx->getVoidType()),
                name
            ));
        case IntrinsicName::StackGet:
            return std::unique_ptr<IntrinsicFunction>(new IntrinsicFunction(
                ".intrinsic.stack_get",
                ctx->makeFunctionType({}, ctx->makePointerType(ctx->getVoidType())),
                name
            ));
        case IntrinsicName::StackSet:
            return std::unique_ptr<IntrinsicFunction>(new IntrinsicFunction(
                ".intrinsic.stack_set",
                ctx->makeFunctionType({vptr}, ctx->getVoidType()),
                name
            ));
        default: break;
    }
    return nullptr;
}

}