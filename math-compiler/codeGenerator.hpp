#pragma once

#include "ir.hpp"
#include <unordered_map>

struct CodeGenerator {
	CodeGenerator();
	void initialize();

	const std::vector<u8>& compile(const std::vector<IrOp>& irCode);
	void patchExecutableCodeRipAddresses(u8* code, const u8* data) const;

	void loadConstantOp(const LoadConstantOp& op);
	void addOp(const AddOp& op);
	void multiplyOp(const MultiplyOp& op);

	void emitU8(u8 value);
	void emitI8(i8 value);
	void emitU32(u32 value);
	void emitI32(u32 value);
	void emitRet();
	void dataEmitU8(u8 value);
	void dataEmitU32(u32 value);

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

	enum class ModRMRegisterX64 : u8 {
		RAX = 0b0000,
		RCX = 0b0001,
		RDX = 0b0010,
		RBX = 0b0011,
		RSP = 0b0100,
		RBP = 0b0101,
		RSI = 0b0110,
		RDI = 0b0111,
		R8 = 0b1000,
		R9 = 0b1001,
		R10 = 0b1010,
		R11 = 0b1011,
		R12 = 0b1100,
		R13 = 0b1101,
		R14 = 0b1110,
		R15 = 0b1111
	};

	enum class ModRMRegisterXmm : u8 {
		XMM0 = static_cast<u8>(ModRMRegisterX86::AX),
		XMM1 = static_cast<u8>(ModRMRegisterX86::CX),
		XMM2 = static_cast<u8>(ModRMRegisterX86::DX),
		XMM3 = static_cast<u8>(ModRMRegisterX86::BX),
		XMM4 = static_cast<u8>(ModRMRegisterX86::SP),
		XMM5 = static_cast<u8>(ModRMRegisterX86::BP),
		XMM6 = static_cast<u8>(ModRMRegisterX86::SI),
		XMM7 = static_cast<u8>(ModRMRegisterX86::DI)
	};

	[[nodiscard]] static u8 encodeModRmDirectAddressingByte(u8 g, u8 e);
	[[nodiscard]] static u8 encodeModRmDirectAddressingByte(ModRMRegisterXmm g, ModRMRegisterXmm e);
	[[nodiscard]] static u8 encodeModRmDirectAddressingByte(ModRMRegisterX86 g, ModRMRegisterX86 e);

	[[nodiscard]] static u8 encodeModRmByte(u8 mod /* 2 bit */, u8 reg /* 3 bit */, u8 rm /* 3 bit */);

	// <registerWithAddress> + disp8]
	[[nodiscard]] static u8 encodeModRmReg32DispI8Reg32(ModRMRegisterX86 registerWithAddress, ModRMRegisterX86 reg);
	[[nodiscard]] static u8 encodeModRmReg32DispI32Reg32(ModRMRegisterX86 registerWithAddress, ModRMRegisterX86 reg);

	[[nodiscard]] static u8 encodeRexPrefixByte(bool w, bool r, bool x, bool b);

	void emitAddRegisterToRegister32(ModRMRegisterX86 rhs, ModRMRegisterX86 lhs);
	void emitAddRegisterToRegister64(ModRMRegisterX64 rhs, ModRMRegisterX64 lhs);
	void emitMovssRegToRegXmm(ModRMRegisterXmm destination, ModRMRegisterXmm source);

	void emitAddssRegXmmRegXmm(ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);
	void emitMulssRegXmmRegXmm(ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);

	// !!!!!!!!!!!!!!! TODO: Read about addressing.

	// TODO: Doesn't work with ESP 0b100 in the table
	void emitMovssToRegXmmFromMemReg32DispI8(ModRMRegisterXmm destination, ModRMRegisterX86 regWithSourceAddress, i8 addressDisplacement);
	void emitMovssToRegXmmFromMemReg32DispI32(ModRMRegisterXmm destination, ModRMRegisterX86 regWithSourceAddress, i32 addressDisplacement);

	void emitMovssToMemReg32DispI8FromRegXmm(ModRMRegisterX86 regWithDestinationAddress, i8 addressDisplacement, ModRMRegisterXmm source);
	void emitMovssToMemReg32DispI32FromRegXmm(ModRMRegisterX86 regWithDestinationAddress, i32 addressDisplacement, ModRMRegisterXmm source);
	//void emitMovsMemEbp32ToReg(u32 ebpDisplacement, ModRMRegisterXmm source);
	//void emitMovsMemToRegXmm(ModRMRegisterXmm destination, ModRMRegisterXmm source);
	/*void allocateImmediateAndEmitRipOffset32(float constant);*/
	void movssToRegXmmConstant32(ModRMRegisterXmm destination, float immediate);

	void emitPushReg64(ModRMRegisterX64 reg);
	void emitPopReg64(ModRMRegisterX64 reg);

	struct RipRelativeImmediate32Allocation {
		float value;
		i64 ripOffsetBytesCodeIndex; // ripOffset is at code.data() + ripOffsetBytesCodeIndex
		i64 immediateDataSectionOffset; // immediate is at data.data() + immediateDataSectionOffset
	};
	std::vector<RipRelativeImmediate32Allocation> ripRelativeImmediate32Allocations;
	
	struct StackAllocation {
		i32 baseOffset;
		i32 byteSize;
	};
	std::vector<StackAllocation> stackAllocations;
	i32 stackMemoryAllocated;
	struct BaseOffset {
		i32 baseOffset;
	};
	BaseOffset stackAllocate(i32 size, i32 aligment);

	struct RegisterLocation {
		BaseOffset location;
	};
	std::unordered_map<Register, RegisterLocation> registerLocations;
	RegisterLocation getRegisterLocation(Register reg);

	void movssToRegisterLocationFromRegXmm(const RegisterLocation& to, ModRMRegisterXmm from);
	void movssToRegXmmFromRegisterLocation(ModRMRegisterXmm to, const RegisterLocation& from);

	std::vector<u8> code;
	std::vector<u8> data;
};