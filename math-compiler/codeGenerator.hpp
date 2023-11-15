#pragma once

#include "ir.hpp"

struct CodeGenerator {
	CodeGenerator();
	void initialize();

	const std::vector<u8>& compile(const std::vector<IrOp>& irCode);

	void emitByte(u8 value);
	void emitRet();

	enum class ModRMRegisterX86 : u8 {
		AX = 0b000,
		CX = 0b001,
		DX = 0b010,
		BX = 0b011,
		SP = 0b100,
		BP = 0b101,
		SI = 0b110,
		DI = 0b111
	};

	static u8 encodeModRmDirectAddressingByte(ModRMRegisterX86 g, ModRMRegisterX86 e);
	static u8 encodeModRmByte(u8 mod /* 2 bit */, u8 reg /* 3 bit */, u8 rm /* 3 bit */);
	static u8 encodeModRmDirectAddressingByte(ModRMRegisterX86 g, ModRMRegisterX86 e);
	static u8 encodeRexPrefixByte(bool w, bool r, bool x, bool b);

	void emitAddRegisterToRegister32(ModRMRegisterX86 rhs, ModRMRegisterX86 lhs);

	std::vector<u8> code;
};