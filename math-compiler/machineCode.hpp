#pragma once

#include "assemblyCode.hpp"
#include <unordered_map>
#include <span>

struct MachineCode {
	void initialize();
	void generateFrom(const AssemblyCode& assembly);

	void patchRipRelativeOperands(
		u8* code, 
		const u8* data, 
		const std::unordered_map<AddressLabel, void*>& labelToAddress) const;

	void emitU8(u8 value);
	void emitI8(i8 value);
	void emitU32(u32 value);
	void emitI32(i32 value);
	void emitU64(u64 value);

	void emitModRm(u8 mod, u8 reg, u8 rm);
	void emitModRmDirectAddressing(u8 reg, u8 rm);
	void emitModRmRegDisp(u8 reg, u8 regWithAddress, i32 displacement);

	// w - use a 64 bit operand
	// r - modrm.reg extension
	// x - sib.index extension
	// b - modrm.rm extension
	void emitRex(bool w, bool r, bool x, bool b);

	/*
	w - rex.w or opcode extension
	r - rex.r negated
	x - rex.x negated
	b - rex.b negated
	m-mmmm - opcode bytes {
		00001 0x0F
		00010 0x0F38
		00010 0x0F3A
	}
	vvvv - register (for example for 3 operand instructions) or 1111 if unused
	pp - opcode extension {
		00 None
		01 66
		10 F3
		11 F2
	}
	*/
	void emit2ByteVex(bool r, u8 vvvv, bool l, u8 pp);
	void emit3ByteVex(bool r, bool x, bool b, u8 m_mmmm, bool w, u8 vvvv, bool l, u8 pp);

	void emitInstructionYmmYmmYmm(u8 opCode, u8 a, u8 b, u8 c);
	void emitReg64Reg64Instruction(u8 opCode, Reg64 lhs, Reg64 rhs);
	void emitReg64ImmInstruction(u8 opCode, u8 opCodeExtension, Reg64 lhs, u32 rhs);

	void emit(const CallReg& i);
	void emit(const CallLbl& i);
	void emit(const Ret& i);
	void emit(const Push64& i);
	void emit(const Pop64& i);
	void emit(const JmpLbl& i);
	void emit(const XorR64R64& i);
	void emit(const AndR8Imm& i);
	void emit(const AddR64Imm& i);
	void emit(const SubR64Imm& i);
	void emit(const Inc64& i);
	void emit(const CmpR64R64& i);
	void emit(const MovR64R64& i);
	void emit(const MovR64Imm64& i);
	void emit(const VbroadcastssLbl& i);
	void emit(const VmovapsYmmYmm& i);
	void emitInstructionYmmRegDisp(u8 opCode, u8 reg, u8 regWithAddress, i32 disp);
	void emit(const VmovapsYmmMem& i);
	void emit(const VmovapsMemYmm& i);
	void emit(const VaddpsYmmYmmYmm& i);
	void emit(const VsubpsYmmYmmYmm& i);
	void emit(const VmulpsYmmYmmYmm& i);
	void emit(const VdivpsYmmYmmYmm& i);
	void emit(const VxorpsYmmYmmYmm& i);

	std::vector<u8> code;
	i64 currentLocation();

	void dataEmitU8(u8 value);
	void dataEmitU32(u32 value);
	void dataEmitF32(float value);

	std::vector<u8> data;
	std::vector<i64> dataLabelToDataOffset;
	struct RipRelativeDataOperand {
		i64 operandCodeOffset;
		i64 dataOffset;
	};
	std::vector<RipRelativeDataOperand> ripRelativeDataOperands;

	// Why bother storing nextInstructionCodeOffset if it is just operandCodeOffset.
	struct RipRelativeJump {
		i64 operandCodeOffset;
		i64 nextInstructionCodeOffset;
		AddressLabel label;
	};
	std::vector<RipRelativeJump> ripRelativeJumps;

	// TODO: Maybe rename to local jump.
	struct JumpToPatch {
		i64 instructionSize;
		i64 displacemenOperandBytesCodeOffset;
		InstructionLabel destination;
	};
	std::vector<JumpToPatch> jumpsToPatch;
};