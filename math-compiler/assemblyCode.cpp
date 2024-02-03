#include "assemblyCode.hpp"
#include "utils/overloaded.hpp"

void AssemblyCode::reset() {
	nextInstructionLabel = std::nullopt;
	allocatedLabelsCount = 0;
	instructions.clear();
	dataEntries.clear();
}

void AssemblyCode::ret(i64 offset) {
	insert(Ret{}, offset);
}

void AssemblyCode::push(Reg64 reg, i64 offset) {
	insert(Push64{ .reg = reg }, offset);
}

void AssemblyCode::pop(Reg64 reg, i64 offset) {
	insert(Pop64{ .reg = reg }, offset);
}

void AssemblyCode::cmp(Reg64 lhs, Reg64 rhs, i64 offset) {
	insert(CmpR64R64{ .lhs = lhs, .rhs = rhs }, offset);
}

void AssemblyCode::xor_(Reg64 lhs, Reg64 rhs, i64 offset) {
	insert(XorR64R64{ .lhs = lhs, .rhs = rhs }, offset);
}

void AssemblyCode::and_(Reg8 lhs, u8 rhs, i64 offset) {
	insert(AndR8Imm{ .lhs = lhs, .rhs = rhs }, offset);
}

void AssemblyCode::add(Reg64 lhs, u32 rhs, i64 offset) {
	insert(AddR64Imm{ .lhs = lhs, .rhs = rhs }, offset);
}

void AssemblyCode::sub(Reg64 lhs, u32 rhs, i64 offset) {
	insert(SubR64Imm{ .lhs = lhs, .rhs = rhs }, offset);
}

void AssemblyCode::inc(Reg64 reg, i64 offset) {
	insert(Inc64{ .reg = reg }, offset);
}

void AssemblyCode::vmovaps(RegYmm destiation, RegYmm source, i64 offset) {
	insert(VmovapsYmmYmm{ .destination = destiation, .source = source }, offset);
}

void AssemblyCode::vmovaps(RegYmm destiation, Reg64 sourceAddressReg, i32 addressOffset, i64 offset) {
	insert(VmovapsYmmMem{ .destination = destiation, .sourceAddressReg = sourceAddressReg, .addressOffset = addressOffset }, offset);
}

void AssemblyCode::vmovaps(Reg64 destinationAddressReg, i32 addressOffset, RegYmm source, i64 offset) {
	insert(VmovapsMemYmm{ .destinationAddressReg = destinationAddressReg, .addressOffset = addressOffset, .source = source }, offset);
}

void AssemblyCode::vaddps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset) {
	insert(VaddpsYmmYmmYmm{ .destination = destination, .lhs = lhs, .rhs = rhs }, OFFSET_LAST);
}

void AssemblyCode::vsubps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset) {
	insert(VsubpsYmmYmmYmm{ .destination = destination, .lhs = lhs, .rhs = rhs }, OFFSET_LAST);
}

void AssemblyCode::vmulps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset) {
	insert(VmulpsYmmYmmYmm{ .destination = destination, .lhs = lhs, .rhs = rhs }, OFFSET_LAST);
}

void AssemblyCode::vdivps(RegYmm destination, RegYmm lhs, RegYmm rhs, i64 offset) {
	insert(VdivpsYmmYmmYmm{ .destination = destination, .lhs = lhs, .rhs = rhs }, OFFSET_LAST);
}

void AssemblyCode::jmp(InstructionLabel label, i64 offset) {
	insert(JmpLbl{ .type = JmpType::UNCONDITONAL, .label = label }, offset);
}

void AssemblyCode::jl(InstructionLabel label, i64 offset) {
	insert(JmpLbl{ .type = JmpType::SIGNED_LESS, .label = label }, offset);
}

void AssemblyCode::mov(Reg64 destination, Reg64 source, i64 offset) {
	insert(MovR64R64{ .destination = destination, .source = source }, offset);
}

void AssemblyCode::vbroadcastss(RegYmm destination, DataLabel source, i64 offset) {
	insert(VbroadcastssLbl{ .destination = destination, .source = source }, offset);
}

void AssemblyCode::insert(const Instruction& instruction, i64 offset) {
	LabeledInstruction labeledInstruction{ INSTRUCTION_LABEL_NONE, instruction };
	if (offset == OFFSET_LAST) {
		if (nextInstructionLabel.has_value()) {
			labeledInstruction.label = *nextInstructionLabel;
			nextInstructionLabel = std::nullopt;
		}
		instructions.push_back(labeledInstruction);
	} else {
		instructions.insert(instructions.begin() + offset, labeledInstruction);
	}
}

void AssemblyCode::setLabelOnNextInstruction(InstructionLabel label) {
	nextInstructionLabel = label;
}

InstructionLabel AssemblyCode::allocateLabel() {
	const auto label = allocatedLabelsCount;
	allocatedLabelsCount++;
	return label;
}

DataLabel AssemblyCode::allocateData(float value) {
	const DataLabel label = i32(dataEntries.size());
	dataEntries.push_back(DataEntry{ .value = value});
	return label;
}

u8 regIndex(Reg64 reg) {
	return u8(reg);
}

u8 regIndex(RegYmm reg) {
	return u8(reg);
}

RegYmm regYmmFromIndex(u8 index) {
	return RegYmm(index);
}
