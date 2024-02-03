#include "machineCode.hpp"
#include "utils/asserts.hpp"
#include <bit>
#include <unordered_map>

static u8 takeFirst3Bits(u8 r) {
	return 0b111 & u8(r);
}

bool take4thBit(u8 r) {
	return (u8(r) >> 3) & 0b1;
}

void MachineCode::initialize() {
	code.clear();
	data.clear();
	dataLabelToDataOffset.clear();
	ripRelativeOperands.clear();
	jumpsToPatch.clear();
}

void MachineCode::generateFrom(const AssemblyCode& assembly) {
	for (DataLabel label = 0; label < assembly.dataEntries.size(); label++) {
		const auto& entry = assembly.dataEntries[label];
		const auto offset = data.size();
		dataEmitF32(entry.value);
		dataLabelToDataOffset.push_back(offset);
	}

	std::unordered_map<InstructionLabel, i64> labelToCodeOffset;

	for (const auto& [label, instruction] : assembly.instructions) {
		if (label != INSTRUCTION_LABEL_NONE) {
			const bool insertedNewElement = labelToCodeOffset.insert({ label, currentLocation() }).second;
			// A single label shouldn't be in 2 places.
			ASSERT(insertedNewElement);
		}
		std::visit([this](auto&& i) -> void { emit(i); }, instruction);
	}

	// encoding jumps
	// https://stackoverflow.com/questions/14889643/how-encode-a-relative-short-jmp-in-x86
	for (const auto& jump : jumpsToPatch) {
		const auto destinationIt = labelToCodeOffset.find(jump.destination);
		// The destination label has to exist.
		ASSERT(destinationIt != labelToCodeOffset.end());
		const auto destination = destinationIt->second;

		const auto operandCodeOffset = code.data() + jump.displacemenOperandBytesCodeOffset;
		const auto instructionFirstByte = jump.displacemenOperandBytesCodeOffset - (jump.instructionSize - sizeof(i32));
		const auto displacement = destination - instructionFirstByte;
		const i32 operand = displacement - jump.instructionSize;
		memcpy(operandCodeOffset, &operand, sizeof(i32));
	}
}

void MachineCode::patchRipRelativeOperands(u8* code, const u8* data) const {
	for (const auto& allocation : ripRelativeOperands) {
		const auto ripOffsetAddress = code + allocation.operandCodeOffset;
		const auto dataOffsetAddress = data + allocation.dataOffset;
		/*const auto ripOffsetAddress = code + allocation.ripOffsetBytesCodeIndex;
		const auto dataOffsetAddress = data + allocation.immediateDataSectionOffset;*/
		float data;
		memcpy(&data, dataOffsetAddress, sizeof(data));
		const auto ripOffsetSize = sizeof(i32);
		// RIP addresses relative to the start byte of the next instruction.
		// The part of the instruction is the 32 bit rip offset so the first byte of the next instruction is 
		const auto nextInstructionAddress = ripOffsetAddress + ripOffsetSize;

		// If there are any issues with this code it might have something to do with the sign of the offset.
		i32 ripOffsetToData = static_cast<i32>(dataOffsetAddress - nextInstructionAddress);
		memcpy(ripOffsetAddress, &ripOffsetToData, ripOffsetSize);

		ASSERT(dataOffsetAddress == nextInstructionAddress + ripOffsetToData);
	}
}

void MachineCode::emitU8(u8 value) {
	code.push_back(value);
}

void MachineCode::emitI8(i8 value) {
	emitU8(std::bit_cast<u8>(value));
}

void MachineCode::emitU32(u32 value) {
	const auto bytes = reinterpret_cast<u8*>(&value);
	emitU8(bytes[0]);
	emitU8(bytes[1]);
	emitU8(bytes[2]);
	emitU8(bytes[3]);
}

void MachineCode::emitI32(i32 value) {
	emitU32(std::bit_cast<u32>(value));
}

void MachineCode::emitModRm(u8 mod, u8 reg, u8 rm) {
	emitU8(
		((mod & 0b11) << 6) |
		((reg & 0b111) << 3) | 
		(rm & 0b111));
}

void MachineCode::emitModRmDirectAddressing(u8 reg, u8 rm) {
	emitModRm(0b11, reg, rm);
}

void MachineCode::emitModRmRegDisp(u8 reg, u8 regWithAddress, i32 displacement) {
	if (displacement >= INT8_MIN && displacement <= INT8_MAX) {
		emitModRm(0b01, reg, regWithAddress);
		emitI8(i8(displacement));
	} else {
		emitModRm(0b10, reg, regWithAddress);
		emitI32(displacement);
	}
}

void MachineCode::emitRex(bool w, bool r, bool x, bool b) {
	emitU8(0b0100'0000 | (u8(w) << 3) | (u8(r) << 2) | (u8(x) << 1) | u8(b));
}

void MachineCode::emit2ByteVex(bool r, u8 vvvv, bool l, u8 pp) {
	emitU8(0xC5);
	emitU8(
		(u8(r) << 7) |
		((vvvv & 0b1111) << 3) |
		(u8(l) << 2) |
		(pp & 0b11));
}

void MachineCode::emit3ByteVex(bool r, bool x, bool b, u8 m_mmmm, bool w, u8 vvvv, bool l, u8 pp) {
	emitU8(0xC4);
	emitU8(
		(u8(r) << 7) |
		(u8(x) << 6) |
		(u8(b) << 5) |
		(m_mmmm & 0b11111));
	emitU8(
		(u8(w) << 7) |
		((vvvv & 0b1111) << 3) |
		(u8(l) << 2) |
		(pp & 0b11));
}

void MachineCode::emitInstructionYmmYmmYmm(u8 opCode, u8 a, u8 b, u8 c) {
	const auto a4thBit = take4thBit(a);
	const auto c4thBit = take4thBit(c);

	const auto negatedB = ~static_cast<u8>(b) & 0b1111;
	//emit2ByteVex(1, invertedLhs, 1, 0b00);
	if (c4thBit == 0) {
		emit2ByteVex(!a4thBit, negatedB, 1, 0b00);
	} else {
		emit3ByteVex(!a4thBit, 0, !c4thBit, 0b000001, 0, negatedB, 1, 0b00);
	}
	emitU8(opCode);
	emitModRmDirectAddressing(takeFirst3Bits(a), takeFirst3Bits(c));
}

void MachineCode::emitReg64Reg64Instruction(u8 opCode, Reg64 lhs, Reg64 rhs) {
	emitRex(1, take4thBit(regIndex(lhs)), 0, take4thBit(regIndex(rhs)));
	emitU8(opCode);
	emitModRmDirectAddressing(takeFirst3Bits(regIndex(lhs)), takeFirst3Bits(regIndex(rhs)));
}

void MachineCode::emitReg64ImmInstruction(u8 opCode, u8 opCodeExtension, Reg64 lhs, u32 rhs){
	emitRex(1, take4thBit(regIndex(lhs)), 0, 0);
	emitU8(opCode);
	emitModRm(0b11, opCodeExtension, takeFirst3Bits(regIndex(lhs)));
	emitU32(rhs);
}

void MachineCode::emit(const Ret& i) {
	emitU8(0xC3);
}

void MachineCode::emit(const Push64& i) {
	emitU8(0x50 + regIndex(i.reg));
}

void MachineCode::emit(const Pop64& i) {
	emitU8(0x58 + regIndex(i.reg));
}

void MachineCode::emit(const JmpLbl& i) {
	i64 instructionSize;

	switch (i.type) {
		using enum JmpType;

	case UNCONDITONAL:
		emitU8(0xE9);
		instructionSize = 5;
		break;

	case SIGNED_LESS:
		emitU8(0x0F);
		emitU8(0x8C);
		instructionSize = 6;
		break;
	}
	const auto operandLocation = currentLocation();
	emitU32(0);

	jumpsToPatch.push_back(JumpToPatch{
		.instructionSize = instructionSize,
		.displacemenOperandBytesCodeOffset = operandLocation,
		.destination = i.label
	});
}

void MachineCode::emit(const XorR64R64& i) {
	emitReg64Reg64Instruction(0x33, i.lhs, i.rhs);
}

void MachineCode::emit(const AndR8Imm& i) {
	switch (i.lhs) {	
	case Reg8::BPL:
		emitRex(0, 0, 0, 0);
		emitU8(0x80);
		emitModRm(0b11, 0x4, 5);
		emitU8(i.rhs);
		break;
	}
}

void MachineCode::emit(const AddR64Imm& i) {
	emitReg64ImmInstruction(0x81, 0x0, i.lhs, i.rhs);
}

void MachineCode::emit(const SubR64Imm& i) {
	emitReg64ImmInstruction(0x81, 0x5, i.lhs, i.rhs);
}

void MachineCode::emit(const Inc64& i) {
	emitRex(1, 0, 0, take4thBit(regIndex(i.reg)));
	emitU8(0xFF);
	emitModRm(0b11, 0b000, takeFirst3Bits(regIndex(i.reg)));
}

void MachineCode::emit(const CmpR64R64& i) {
	// For some reason cmp has reversed operands. That is the assembly order is reversed compared to the machine code order.
	emitReg64Reg64Instruction(0x39, i.rhs, i.lhs);
}

void MachineCode::emit(const MovR64R64& i) {
	const auto destination = regIndex(i.destination);
	const auto source = regIndex(i.source);
	emitRex(1, take4thBit(destination), 0, take4thBit(source));
	emitU8(0x89);
	emitModRmDirectAddressing(takeFirst3Bits(destination), takeFirst3Bits(source));
}

void MachineCode::emit(const VbroadcastssLbl& i) {
	const auto destination = regIndex(i.destination);
	const auto destination4thBit = take4thBit(destination);

	emit3ByteVex(!destination4thBit, 0, 0, 0b00010, 0, 0b1111, 1, 0b01);
	emitU8(0x18);
	emitModRm(0b00, static_cast<u8>(takeFirst3Bits(destination)), 0b101);
	const i64 ripOffsetBytesCodeOffset = code.size(); // Points to last by of the current code. Which is where the next bytes begin.
	const auto operandCodeOffset = currentLocation();
	emitU32(0);

	ASSERT(i.source < dataLabelToDataOffset.size());
	const auto dataOffset = dataLabelToDataOffset[i.source];

	ripRelativeOperands.push_back(RipRelativeOperand{
		.operandCodeOffset = operandCodeOffset,
		.dataOffset = dataOffset,
	});
}

void MachineCode::emit(const VmovapsYmmYmm& i) {
	emitInstructionYmmYmmYmm(0x29, regIndex(i.destination), 0b0000, regIndex(i.source));
}

void MachineCode::emitInstructionYmmRegDisp(u8 opCode, u8 reg, u8 regWithAddress, i32 disp) {
	emit2ByteVex(!take4thBit(reg), 0b1111, 1, 0b00);
	emitU8(opCode);
	emitModRmRegDisp(takeFirst3Bits(reg), takeFirst3Bits(regWithAddress), disp);
}

// TODO: Could use a different encoding if the offset is 0.
void MachineCode::emit(const VmovapsYmmMem& i) {
	emitInstructionYmmRegDisp(0x28, regIndex(i.destination), regIndex(i.sourceAddressReg), i.addressOffset);
}

void MachineCode::emit(const VmovapsMemYmm& i) {
	emitInstructionYmmRegDisp(0x29, regIndex(i.source), regIndex(i.destinationAddressReg), i.addressOffset);
}

void MachineCode::emit(const VaddpsYmmYmmYmm& i) {
	emitInstructionYmmYmmYmm(0x58, regIndex(i.destination), regIndex(i.lhs), regIndex(i.rhs));
}

void MachineCode::emit(const VsubpsYmmYmmYmm& i) {
	emitInstructionYmmYmmYmm(0x5C, regIndex(i.destination), regIndex(i.lhs), regIndex(i.rhs));
}

void MachineCode::emit(const VmulpsYmmYmmYmm& i) {
	emitInstructionYmmYmmYmm(0x59, regIndex(i.destination), regIndex(i.lhs), regIndex(i.rhs));
}

void MachineCode::emit(const VdivpsYmmYmmYmm& i) {
	emitInstructionYmmYmmYmm(0x5E, regIndex(i.destination), regIndex(i.lhs), regIndex(i.rhs));
}

i64 MachineCode::currentLocation() {
	return code.size();
}

void MachineCode::dataEmitU8(u8 value) {
	data.push_back(value);
}

void MachineCode::dataEmitU32(u32 value) {
	const auto bytes = reinterpret_cast<u8*>(&value);
	dataEmitU8(bytes[0]);
	dataEmitU8(bytes[1]);
	dataEmitU8(bytes[2]);
	dataEmitU8(bytes[3]);
}

void MachineCode::dataEmitF32(float value) {
	dataEmitU32(std::bit_cast<u32>(value));
}
