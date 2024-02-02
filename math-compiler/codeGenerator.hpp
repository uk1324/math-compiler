#pragma once

#include "ir.hpp"
#include "input.hpp"
#include <unordered_map>
#include <span>
#include <optional>
#include <bitset>

// Vex prefix described on page 538 on the intel manual.
// also https://wiki.osdev.org/X86-64_Instruction_Encoding#VEX.2FXOP_opcodes
// If you for example have 
// VEX.128.0F.WIG 28 /r 
// 0F probably refers to the value of the m-mmmm field (this isn't the actual value of the field read the manual to find the values).
// the l bit of the prefix decides wether it is a 128 or 256 bit instruction
// the /r mean the modrm field

// Decoding REX.W + 81 /0 id
// Set the REX.W = 1
// emit 0x81
// /0 mean that the modrm.reg = 0b000
// id mean immediate double word.

/*

void perform_calculations(const float* input, float* output);

extern const i64 INPUT_BATCH_SIZE;
extern const i64 OUTPUT_BATCH_SIZE;

void function(const float* input, float* output, i64 outputBatchCount) {
	for (i64 i = 0; i < outputBatchCount; i++) {
		perform_calculations(input, output);

		input += INPUT_BATCH_SIZE;
		output += OUTPUT_BATCH_SIZE;
	}
}
*/

/*
mov some_free_register, 0 // Could do this using xor
jmp loop_check
loop_start:

// loop content

add integer_input_register[0], INPUT_BATCH_SIZE
add integer_input_register[1], OUTPUT_BATCH_SIZE
inc some_free_register
loop_check:
cmp some_free_register, integer_input_register[2]
jl loop_start
ret

*/

// TODO: Could create an assembler class.
// Could emit struct instead of emiting code directly so the assembly could be optimized using peephole optimization.
// The patching of jumps would still need to work the way it is currently implemented, because if there is a forward jump then the relative distance is still unknown so the jump must be patched after all the code is emmited.

// TODO: Maybe make a function that just returns the lower part of a register64 and return a register32 with error checking.
struct CodeGenerator {
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
		XMM7 = static_cast<u8>(ModRMRegisterX86::DI),
		// TODO: Not all function handle arguments with indicies bigger than 7
		XMM8 = static_cast<u8>(ModRMRegisterX64::R8),
		XMM9 = static_cast<u8>(ModRMRegisterX64::R9),
		XMM10 = static_cast<u8>(ModRMRegisterX64::R10),
		XMM11 = static_cast<u8>(ModRMRegisterX64::R11),
		XMM12 = static_cast<u8>(ModRMRegisterX64::R12),
		XMM13 = static_cast<u8>(ModRMRegisterX64::R13),
		XMM14 = static_cast<u8>(ModRMRegisterX64::R14),
		XMM15 = static_cast<u8>(ModRMRegisterX64::R15),
	};
	static constexpr i64 XMM_REGISTER_COUNT = 16;

	enum class Register8Bit {
		AH,
		AL,
		BH,
		BL,
		CH,
		CL,
		SPL,
		BPL,
		DIL,
		SIL,
		DH,
		DL
	};

	/*
	windows integer arguments
	rcx, rdx, r8, r9

	system v integer arguments
	rdi, rsi, rdx, rcx, r8, r9
	*/

	static constexpr ModRMRegisterX64 INTEGER_FUNCTION_INPUT_REGISTERS[]{
		ModRMRegisterX64::RCX,
		ModRMRegisterX64::RDX,
		ModRMRegisterX64::R8,
		ModRMRegisterX64::R9,
	};

	CodeGenerator();
	void initialize(std::span<const FunctionParameter> parameters);

	const std::vector<u8>& compile(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters);

	// Emmiting jumps after the code has been generated is can be difficult in some situations.
	/*
	For example if you have something like this
	b:
	...
	jmp a
	...
	jmp b
	a:
	...
	Then the size of the relative distance of a depends on the size of relative distance of b and vice versa. Not sure how this could be optimized, but another simpler method might be to take the maximum possible distance based on the number of jumps between the label and the jump and base the size of the jump displacement on that. The simplest thing would be to always emit the biggest instruction size unless you know already know the size of the operand when emmiting the instruction.

	I guess one way that might work might be first emmiting all the instructions with the max size then iterating untill nothing changes and replacing the bigger sizes with smaller ones. I don't think anything should break, because the distance can only get smaller if you make the operands of outher instructions smaller.
	*/
	i64 currentInstructionIndex = 0;

	std::span<const FunctionParameter> parameters;

	void computeRegisterLastUsage(const std::vector<IrOp>& irCode);
	std::unordered_map<Register, i64> registerToLastUsage;

	struct RegisterConstantOffsetLocation {
		ModRMRegisterX86 registerWithAddress;
		i64 offset;
	};

	struct ConstantLocation {
		float value;
	};
	using MemoryLocation = std::variant<RegisterConstantOffsetLocation, ConstantLocation>;

	struct DataLocation {
		// The state where both are std::nullopt shouldn't happen.
		std::optional<MemoryLocation> memoryLocation;
		std::optional<ModRMRegisterXmm> registerLocation;
	};

	static constexpr i64 YMM_REGISTER_SIZE = 8 * sizeof(float);


	std::unordered_map<Register, DataLocation> registerToLocation;
	std::optional<Register> registerAllocations[XMM_REGISTER_COUNT];
	// It might make sense to use a priority queue so certain registers are allocated over others.

	void movToYmmFromMemoryLocation(ModRMRegisterXmm destination, const MemoryLocation& memoryLocation);

	static constexpr i64 CALLER_SAVED_REGISTER_COUNT = 5;
	std::bitset<XMM_REGISTER_COUNT - CALLER_SAVED_REGISTER_COUNT> calleSavedRegisters;
	
	// Could also make functions that return a register or a memory location.
	// Also there could be a version that allocates 2 data locations at once that could be used for commutative operations.
	ModRMRegisterXmm getRegisterLocationHelper(Register reg);
	ModRMRegisterXmm getRegisterLocation(Register reg);

	void patchJumps();

	void patchExecutableCodeRipAddresses(u8* code, const u8* data) const;

	void loadConstantOp(const LoadConstantOp& op);
	void addOp(const AddOp& op);
	void multiplyOp(const MultiplyOp& op);
	void returnOp(const ReturnOp& op);

	void emitU8(u8 value);
	void emitI8(i8 value);
	void emitU32(u32 value);
	void emitI32(u32 value);
	void emitRet();
	void dataEmitU8(u8 value);
	void dataEmitU32(u32 value);

	[[nodiscard]] static u8 encodeModRmDirectAddressingByte(u8 g, u8 e);
	[[nodiscard]] static u8 encodeModRmDirectAddressingByte(ModRMRegisterXmm g, ModRMRegisterXmm e);
	[[nodiscard]] static u8 encodeModRmDirectAddressingByte(ModRMRegisterX86 g, ModRMRegisterX86 e);

	[[nodiscard]] static u8 encodeModRmByte(u8 mod /* 2 bit */, u8 reg /* 3 bit */, u8 rm /* 3 bit */);

	// <registerWithAddress> + disp8]
	[[nodiscard]] static u8 encodeModRmReg32DispI8Reg32(ModRMRegisterX86 registerWithAddress, ModRMRegisterX86 reg);
	[[nodiscard]] static u8 encodeModRmReg32DispI32Reg32(ModRMRegisterX86 registerWithAddress, ModRMRegisterX86 reg);

	[[nodiscard]] static u8 encodeRexPrefixByte(bool w, bool r, bool x, bool b);

	void emitMovToReg32FromReg32(ModRMRegisterX86 destination, ModRMRegisterX86 source);
	void emitMovToReg64FromReg64(ModRMRegisterX64 destination, ModRMRegisterX64 source);

	void emitModRmReg32DispI32Reg32(ModRMRegisterX86 registerWithAddress, i32 displacement, ModRMRegisterX86 reg);

	void emitAddRegister32ToRegister32(ModRMRegisterX86 rhs, ModRMRegisterX86 lhs);
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

	void emit2ByteVex(bool r, u8 vvvv, bool l, u8 pp);
	void emit3ByteVex(bool r, bool x, bool b, u8 m_mmmm, bool w, u8 vvvv, bool l, u8 pp);
	void emitRegYmmRegYmmVexByte(ModRMRegisterXmm a, ModRMRegisterXmm b);

	void emitInstructionRegYmmRegYmmRegYmm(u8 opCode, ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);

	//void emitVmovssToRegXmmFromRipOffset()

	void emitVmovapsToRegYmmFromRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm source);
	void emitVmovapsToMemReg32DispI32FromRegYmm(ModRMRegisterX86 regWithDestinationAddress, i32 addressDisplacement, ModRMRegisterXmm source);
	void emitVmovapsToRegYmmFromMemReg32DispI32(ModRMRegisterXmm destination, ModRMRegisterX86 regWithSourceAddress, i32 addressDisplacement);
	void loadRegYmmConstant32(ModRMRegisterXmm destination, float immediate);

	void emitShufpsYmm(ModRMRegisterXmm ymm1, ModRMRegisterXmm ymm2, ModRMRegisterXmm ymm3, u8 immediate);

	void emitVaddpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);
	void emitVsubpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);
	void emitVmulpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);
	void emitVdivpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs);

	/*void emitJmpRipRelative(i32 displacement);
	void emitJlRipRelative(i32 displacement);*/

	enum class JumpType {
		UNCONDITONAL,
		SIGNED_LESS
	};

	struct JumpInfo {
		i64 instructionSize;
		i64 displacementBytesCodeOffset;
	};
	[[nodiscard]] JumpInfo emitJump(JumpType type);
	void emitAndPatchJump(JumpType type, i64 jumpDestination);
	void patchJump(const JumpInfo& info, i64 jumpDestination);
	i64 currentCodeLocation() const;

	struct JumpToPatch {
		JumpInfo jump;
		i64 jumpDestination;
	};
	std::vector<JumpToPatch> jumpsToPatch;

	void emitCmpReg32Reg32(ModRMRegisterX86 a, ModRMRegisterX86 b);
	void emitCmpReg64Reg64(ModRMRegisterX64 a, ModRMRegisterX64 b);

	void emitAddReg32Imm32(ModRMRegisterX86 destination, i32 immediate);
	void emitAddReg64Imm32(ModRMRegisterX64 destination, i32 immediate);

	void emitSubReg32Imm32(ModRMRegisterX86 destination, i32 immediate);
	void emitSubReg64Imm32(ModRMRegisterX64 destination, i32 immediate);

	void emitIncReg32(ModRMRegisterX86 x);
	void emitIncReg64(ModRMRegisterX64 x);

	// TODO: ModRMRegisterX86 is stupid, because you can't use the lower parts of r8, r9, ... regsiters in these functions.
	// Should probably create some general register type and constrain some function to only take a subset of register by creating another type like Register4Bit and Register3Bit idk.
	void emitXorReg32Reg32(ModRMRegisterX86 lhs, ModRMRegisterX86 rhs);
	void emitXorReg64Reg64(ModRMRegisterX64 lhs, ModRMRegisterX64 rhs);

	void emitAndReg8Imm8(Register8Bit destination, u8 immediate);

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

	//struct RegisterLocation {
	//	BaseOffset location;
	//};
	//std::unordered_map<Register, RegisterLocation> registerLocations;
	//RegisterLocation getRegisterLocation(Register reg);

	/*void movssToRegisterLocationFromRegXmm(const RegisterLocation& to, ModRMRegisterXmm from);
	void movssToRegXmmFromRegisterLocation(ModRMRegisterXmm to, const RegisterLocation& from);*/

	std::vector<u8> code;
	std::vector<u8> data;
};