#pragma once

#include "ir.hpp"
#include "input.hpp"
//#include "assemblyCode.hpp"
#include "machineCode.hpp"
#include <unordered_map>
#include <unordered_set>
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
	/*
	windows integer arguments
	rcx, rdx, r8, r9

	system v integer arguments
	rdi, rsi, rdx, rcx, r8, r9
	*/

	static constexpr Reg64 INTEGER_FUNCTION_INPUT_REGISTERS[]{
		Reg64::RCX,
		Reg64::RDX,
		Reg64::R8,
		Reg64::R9,
	};

	CodeGenerator();
	void initialize(std::span<const FunctionParameter> parameters);

	/*const std::vector<u8>& compile(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters);*/
	MachineCode compile(const std::vector<IrOp>& irCode, std::span<const FunctionParameter> parameters);

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

	void computeRegisterFirstAssigned(const std::vector<IrOp>& irCode);
	std::unordered_map<Register, i64> registerToFirstAssigned;

	struct RegisterConstantOffsetLocation {
		Reg64 registerWithAddress;
		i64 offset;
	};

	struct ConstantLocation {
		float value;
	};
	using MemoryLocation = std::variant<RegisterConstantOffsetLocation, ConstantLocation>;

	struct DataLocation {
		// The state where both are std::nullopt shouldn't happen.
		std::optional<MemoryLocation> memoryLocation;
		std::optional<RegYmm> registerLocation;
	};

	static constexpr i64 YMM_REGISTER_SIZE = 8 * sizeof(float);


	std::unordered_map<Register, DataLocation> virtualRegisterToLocation;
	std::optional<Register> registerAllocations[YMM_REGISTER_COUNT];
	// It might make sense to use a priority queue so certain registers are allocated over others.

	void movToYmmFromMemoryLocation(RegYmm destination, const MemoryLocation& memoryLocation);

	static constexpr i64 CALLER_SAVED_REGISTER_COUNT = 5;
	std::bitset<YMM_REGISTER_COUNT - CALLER_SAVED_REGISTER_COUNT> calleSavedRegisters;
	
	// Could also make functions that return a register or a memory location.
	// Also there could be a version that allocates 2 data locations at once that could be used for commutative operations.
	RegYmm getRegisterLocationHelper(Register reg, std::span<const Register> virtualRegistersThatCanNotBeSpilled);
	RegYmm getRegisterLocation(Register reg, std::span<const Register> virtualRegistersThatCanNotBeSpilled = std::span<const Register>());
	RegYmm allocateRegister(std::span<const Register> virtualRegistersThatCanNotBeSpilled);

	void loadConstantOp(const LoadConstantOp& op);
	void addOp(const AddOp& op);
	void subtractOp(const SubtractOp& op);
	void multiplyOp(const MultiplyOp& op);
	void divideOp(const DivideOp& op);
	void generate(const XorOp& op);
	void generate(const NegateOp& op);
	void returnOp(const ReturnOp& op);

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

	AssemblyCode a;
};