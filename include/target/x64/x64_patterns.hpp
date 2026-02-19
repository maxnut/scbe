#pragma once

#include "MIR/operand.hpp"
#include "ISel/pattern.hpp"

#define OPCODE(op) (uint32_t)Opcode::op

namespace scbe::Target::x64 {

bool matchFunctionArguments(MATCHER_ARGS);
MIR::Operand* emitFunctionArguments(EMITTER_ARGS);

bool matchRegister(MATCHER_ARGS);
MIR::Operand* emitRegister(EMITTER_ARGS);

bool matchFrameIndex(MATCHER_ARGS);
MIR::Operand* emitFrameIndex(EMITTER_ARGS);

bool matchRoot(MATCHER_ARGS);
MIR::Operand* emitRoot(EMITTER_ARGS);

bool matchConstantInt(MATCHER_ARGS);
MIR::Operand* emitConstantInt(EMITTER_ARGS);

bool matchMultiValue(MATCHER_ARGS);
MIR::Operand* emitMultiValue(EMITTER_ARGS);

bool matchExtractValue(MATCHER_ARGS);
MIR::Operand* emitExtractValue(EMITTER_ARGS);

bool matchDynamicAllocation(MATCHER_ARGS);
MIR::Operand* emitDynamicAllocation(EMITTER_ARGS);

bool matchReturn(MATCHER_ARGS);
MIR::Operand* emitReturn(EMITTER_ARGS);

bool matchReturnOp(MATCHER_ARGS);
MIR::Operand* emitReturnLowering(EMITTER_ARGS);

bool matchStoreInFrame(MATCHER_ARGS);
MIR::Operand* emitStoreInFrame(EMITTER_ARGS);

bool matchStoreInPtrRegister(MATCHER_ARGS);
MIR::Operand* emitStoreInPtrRegister(EMITTER_ARGS);

bool matchLoadFromFrame(MATCHER_ARGS);
MIR::Operand* emitLoadFromFrame(EMITTER_ARGS);

bool matchLoadFromPtrRegister(MATCHER_ARGS);
MIR::Operand* emitLoadFromPtrRegister(EMITTER_ARGS);

bool matchAddImmediates(MATCHER_ARGS);
MIR::Operand* emitAddImmediates(EMITTER_ARGS);

bool matchAddRegisters(MATCHER_ARGS);
MIR::Operand* emitAddRegisters(EMITTER_ARGS);

bool matchAddRegistersLea(MATCHER_ARGS);
MIR::Operand* emitAddRegistersLea(EMITTER_ARGS);

bool matchAddRegisterImmediate(MATCHER_ARGS);
MIR::Operand* emitAddRegisterImmediate(EMITTER_ARGS);

bool matchSubImmediates(MATCHER_ARGS);
MIR::Operand* emitSubImmediates(EMITTER_ARGS);

bool matchSubRegisters(MATCHER_ARGS);
MIR::Operand* emitSubRegisters(EMITTER_ARGS);

bool matchSubRegisterImmediate(MATCHER_ARGS);
MIR::Operand* emitSubRegisterImmediate(EMITTER_ARGS);

bool matchPhi(MATCHER_ARGS);
MIR::Operand* emitPhi(EMITTER_ARGS);

bool matchJump(MATCHER_ARGS);
MIR::Operand* emitJump(EMITTER_ARGS);

bool matchCondJumpImmediate(MATCHER_ARGS);
MIR::Operand* emitCondJumpImmediate(EMITTER_ARGS);

bool matchCondJumpRegister(MATCHER_ARGS);
MIR::Operand* emitCondJumpRegister(EMITTER_ARGS);

bool matchCondJumpComparisonII(MATCHER_ARGS);
MIR::Operand* emitCondJumpComparisonII(EMITTER_ARGS);

bool matchCondJumpComparisonRI(MATCHER_ARGS);
MIR::Operand* emitCondJumpComparisonRI(EMITTER_ARGS);

bool matchCondJumpComparisonRR(MATCHER_ARGS);
MIR::Operand* emitCondJumpComparisonRR(EMITTER_ARGS);

bool matchFCondJumpComparisonRF(MATCHER_ARGS);
bool matchFCondJumpComparisonRR(MATCHER_ARGS);
MIR::Operand* emitFCondJumpComparisonRR(EMITTER_ARGS);

bool matchCmpRegisterImmediate(MATCHER_ARGS);
MIR::Operand* emitCmpRegisterImmediate(EMITTER_ARGS);

bool matchCmpRegisterRegister(MATCHER_ARGS);
MIR::Operand* emitCmpRegisterRegister(EMITTER_ARGS);

bool matchCmpImmediateImmediate(MATCHER_ARGS);
MIR::Operand* emitCmpImmediateImmediate(EMITTER_ARGS);

bool matchFCmpRegisterFloat(MATCHER_ARGS);
bool matchFCmpRegisterRegister(MATCHER_ARGS);
MIR::Operand* emitFCmpRegisterRegister(EMITTER_ARGS);

bool matchConstantFloat(MATCHER_ARGS);
MIR::Operand* emitConstantFloat(EMITTER_ARGS);

bool matchGEP(MATCHER_ARGS);
MIR::Operand* emitGEP(EMITTER_ARGS);

bool matchSwitch(MATCHER_ARGS);
MIR::Operand* emitSwitchLowering(EMITTER_ARGS);

bool matchCall(MATCHER_ARGS);
MIR::Operand* emitCallLowering(EMITTER_ARGS);

bool matchIntrinsicCall(MATCHER_ARGS);
MIR::Operand* emitIntrinsicCall(EMITTER_ARGS);

bool matchGlobalValue(MATCHER_ARGS);
MIR::Operand* emitGlobalValue(EMITTER_ARGS);

bool matchZextTo64(MATCHER_ARGS);
MIR::Operand* emitZextTo64(EMITTER_ARGS);

bool matchZextTo32(MATCHER_ARGS);
MIR::Operand* emitZextTo32(EMITTER_ARGS);

bool matchZextTo16(MATCHER_ARGS);
MIR::Operand* emitZextTo16(EMITTER_ARGS);

bool matchZextTo8(MATCHER_ARGS);
MIR::Operand* emitZextTo8(EMITTER_ARGS);

bool matchSextTo64(MATCHER_ARGS);
MIR::Operand* emitSextTo64(EMITTER_ARGS);

bool matchSextTo32(MATCHER_ARGS);
MIR::Operand* emitSextTo32(EMITTER_ARGS);

bool matchSextTo16(MATCHER_ARGS);
MIR::Operand* emitSextTo16(EMITTER_ARGS);

bool matchSextTo8(MATCHER_ARGS);
MIR::Operand* emitSextTo8(EMITTER_ARGS);

bool matchTrunc(MATCHER_ARGS);
MIR::Operand* emitTrunc(EMITTER_ARGS);

bool matchFptrunc(MATCHER_ARGS);
MIR::Operand* emitFptrunc(EMITTER_ARGS);

bool matchFpext(MATCHER_ARGS);
MIR::Operand* emitFpext(EMITTER_ARGS);

bool matchFptosi(MATCHER_ARGS);
MIR::Operand* emitFptosi(EMITTER_ARGS);

bool matchFptoui(MATCHER_ARGS);
MIR::Operand* emitFptoui(EMITTER_ARGS);

bool matchSitofp(MATCHER_ARGS);
MIR::Operand* emitSitofp(EMITTER_ARGS);

bool matchUitofp(MATCHER_ARGS);
MIR::Operand* emitUitofp(EMITTER_ARGS);

bool matchShiftLeftImmediate(MATCHER_ARGS);
MIR::Operand* emitShiftLeftImmediate(EMITTER_ARGS);

bool matchShiftLeftImmediateInv(MATCHER_ARGS);
MIR::Operand* emitShiftLeftImmediateInv(EMITTER_ARGS);

bool matchShiftLeftRegister(MATCHER_ARGS);
MIR::Operand* emitShiftLeftRegister(EMITTER_ARGS);

bool matchLShiftRightImmediate(MATCHER_ARGS);
MIR::Operand* emitLShiftRightImmediate(EMITTER_ARGS);

bool matchLShiftRightImmediateInv(MATCHER_ARGS);
MIR::Operand* emitLShiftRightImmediateInv(EMITTER_ARGS);

bool matchLShiftRightRegister(MATCHER_ARGS);
MIR::Operand* emitLShiftRightRegister(EMITTER_ARGS);

bool matchAShiftRightImmediate(MATCHER_ARGS);
MIR::Operand* emitAShiftRightImmediate(EMITTER_ARGS);

bool matchAShiftRightImmediateInv(MATCHER_ARGS);
MIR::Operand* emitAShiftRightImmediateInv(EMITTER_ARGS);

bool matchAShiftRightRegister(MATCHER_ARGS);
MIR::Operand* emitAShiftRightRegister(EMITTER_ARGS);

bool matchAndImmediate(MATCHER_ARGS);
MIR::Operand* emitAndImmediate(EMITTER_ARGS);

bool matchAndRegister(MATCHER_ARGS);
MIR::Operand* emitAndRegister(EMITTER_ARGS);

bool matchOrImmediate(MATCHER_ARGS);
MIR::Operand* emitOrImmediate(EMITTER_ARGS);

bool matchOrRegister(MATCHER_ARGS);
MIR::Operand* emitOrRegister(EMITTER_ARGS);

bool matchXorImmediate(MATCHER_ARGS);
MIR::Operand* emitXorImmediate(EMITTER_ARGS);

bool matchXorRegister(MATCHER_ARGS);
MIR::Operand* emitXorRegister(EMITTER_ARGS);

bool matchIDiv(MATCHER_ARGS);
MIR::Operand* emitIDiv(EMITTER_ARGS);

bool matchUDiv(MATCHER_ARGS);
MIR::Operand* emitUDiv(EMITTER_ARGS);

bool matchFDiv(MATCHER_ARGS);
MIR::Operand* emitFDiv(EMITTER_ARGS);

bool matchIRem(MATCHER_ARGS);
MIR::Operand* emitIRem(EMITTER_ARGS);

bool matchURem(MATCHER_ARGS);
MIR::Operand* emitURem(EMITTER_ARGS);

bool matchIMulRegisterImmediate(MATCHER_ARGS);
MIR::Operand* emitIMulRegisterImmediate(EMITTER_ARGS);

bool matchIMulRegisterRegister(MATCHER_ARGS);
MIR::Operand* emitIMulRegisterRegister(EMITTER_ARGS);

bool matchUMul(MATCHER_ARGS);
MIR::Operand* emitUMul(EMITTER_ARGS);

bool matchFMul(MATCHER_ARGS);
MIR::Operand* emitFMul(EMITTER_ARGS);

bool matchGenericCast(MATCHER_ARGS);
MIR::Operand* emitGenericCast(EMITTER_ARGS);

}