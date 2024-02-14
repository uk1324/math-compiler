#pragma once

#include "assemblyInstruction.hpp"
#include <vector>
#include <optional>

struct AssemblyCode {
	static constexpr i64 OFFSET_LAST = -1;
	
	void reset();

	void call(Reg64 reg, i64 offset = OFFSET_LAST);
	void call(AddressLabel label, i64 offset = OFFSET_LAST);

	void ret(i64 offset = OFFSET_LAST);

	void push(Reg64 reg, i64 offset = OFFSET_LAST);
	void pop(Reg64 reg, i64 offset = OFFSET_LAST);

	void cmp(Reg64 lhs, Reg64 rhs, i64 offset = OFFSET_LAST);

	void xor_(Reg64 lhs, Reg64 rhs, i64 offset = OFFSET_LAST);
	void and_(Reg8 lhs, u8 rhs, i64 offset = OFFSET_LAST);

	void add(Reg64 lhs, u32 rhs, i64 offset = OFFSET_LAST);
	void sub(Reg64 lhs, u32 rhs, i64 offset = OFFSET_LAST);

	void inc(Reg64 reg, i64 offset = OFFSET_LAST);

	void mov(Reg64 destination, Reg64 source, i64 offset = OFFSET_LAST);
	void mov(Reg64 destination, u64 immediate, i64 offset = OFFSET_LAST);

	void vbroadcastss(RegYmm destination, DataLabel source, i64 offset = OFFSET_LAST);

	void vmovaps(RegYmm destiation, RegYmm source, i64 offset = OFFSET_LAST);
	void vmovaps(RegYmm destiation, Reg64 sourceAddressReg, i32 addressOffset, i64 offset = OFFSET_LAST);
	void vmovaps(Reg64 destinationAddressReg, i32 addressOffset, RegYmm source, i64 offset = OFFSET_LAST);

	void vaddps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset = OFFSET_LAST);
	void vsubps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset = OFFSET_LAST);
	void vmulps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset = OFFSET_LAST);
	void vdivps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset = OFFSET_LAST);

	void vxorps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset = OFFSET_LAST);

	void jmp(InstructionLabel label, i64 offset = OFFSET_LAST);
	// siged less
	void jl(InstructionLabel label, i64 offset = OFFSET_LAST);

	void insert(const Instruction& instruction, i64 offset);

	void setLabelOnNextInstruction(InstructionLabel label);
	std::optional<InstructionLabel> nextInstructionLabel;

	InstructionLabel allocateLabel();
	i32 allocatedLabelsCount = 0;

	DataLabel allocateData(float value);

	std::vector<LabeledInstruction> instructions;

	struct DataEntry {
		float value;
	};
	// DataLabel is the index to this table.
	std::vector<DataEntry> dataEntries;
};
