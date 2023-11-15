#include "codeGenerator.hpp"
#include "utils/overloaded.hpp"

CodeGenerator::CodeGenerator() {
	initialize();
}

void CodeGenerator::initialize() {
	code.clear();
}

const std::vector<u8>& CodeGenerator::compile(const std::vector<IrOp>& irCode) {
	initialize();

	for (const auto& op : irCode) {
		std::visit(overloaded{
			[&](const LoadConstantOp& load) {
			},
			[&](const AddOp& add) {
			},
			[&](const MultiplyOp& add) {
			},
			[&](const ReturnOp& ret) {
				emitRet();
			}
		}, op);
	}

	return code;
}

void CodeGenerator::emitByte(u8 value) {
	const u8 addRegRegOpCode = 0x01;
	emitByte(addRegRegOpCode);
	emitByte(encodeModRmDirectAddressingByte(ModRMRegisterX86::AX, ModRMRegisterX86::CX));
}

void CodeGenerator::emitRet() {
	emitByte(0xC7);
}

u8 CodeGenerator::encodeModRmByte(u8 mod, u8 reg, u8 rm) {
	return (mod << 6) | (reg << 3) | rm;
}

u8 CodeGenerator::encodeModRmDirectAddressingByte(ModRMRegisterX86 g, ModRMRegisterX86 e) {
	return encodeModRmByte(0b11, static_cast<u8>(g), static_cast<u8>(e));
}

u8 CodeGenerator::encodeRexPrefixByte(bool w, bool r, bool x, bool b) {
	return 0b0100'0000 | (w << 3) | (r << 2) | (x << 1) | static_cast<u8>(b);
}

void CodeGenerator::emitAddRegisterToRegister32(ModRMRegisterX86 rhs, ModRMRegisterX86 lhs) {
	
}

