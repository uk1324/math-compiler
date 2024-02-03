#pragma once

#include <variant>
#include "utils/ints.hpp"

// Reg means general purpose register.

enum class Reg64 {
	RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
	R8, R9, R10, R11, R12, R13, R14, R15,
};

enum class Reg32 {
	EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	R8L, R9L, R10L, R11L, R12L, R13L, R14L, R15L,
};

enum class Reg8 {
	BPL, // Lower bits of RBP
};

enum class RegYmm {
	YMM0, YMM1, YMM2, YMM3, YMM4, YMM5, YMM6, YMM7,
	YMM8, YMM9, YMM10, YMM11, YMM12, YMM13, YMM14, YMM15
};

u8 regIndex(Reg64 reg);
u8 regIndex(RegYmm reg);

using InstructionLabel = i32;
const auto INSTRUCTION_LABEL_NONE = ~0;

using DataLabel = i32;

struct Ret {};

struct Push64 {
	Reg64 reg;
};

struct Pop64 {
	Reg64 reg;
};

enum class JmpType {
	UNCONDITONAL,
	SIGNED_LESS
};

struct JmpLbl {
	JmpType type;
	InstructionLabel label;
};

struct XorR64R64 {
	Reg64 lhs;
	Reg64 rhs;
};

struct AndR8Imm {
	Reg8 lhs;
	u8 rhs;
};

struct AddR64Imm {
	Reg64 lhs;
	u32 rhs;
};

struct SubR64Imm {
	Reg64 lhs;
	u32 rhs;
};

struct Inc64 {
	Reg64 reg;
};

struct CmpR64R64 {
	Reg64 lhs;
	Reg64 rhs;
};

struct MovR64R64 {
	Reg64 destination;
	Reg64 source;
};

struct VbroadcastssLbl {
	RegYmm destination;
	DataLabel source;
};

struct VmovapsYmmYmm {
	RegYmm destination;
	RegYmm source;
};

struct VmovapsYmmMem {
	RegYmm destination;
	Reg64 sourceAddressReg;
	i32 addressOffset;
};

struct VmovapsMemYmm {
	Reg64 destinationAddressReg;
	i32 addressOffset;
	RegYmm source;
};

struct VaddpsYmmYmmYmm {
	RegYmm destination;
	RegYmm lhs;
	RegYmm rhs;
};

struct VsubpsYmmYmmYmm {
	RegYmm destination;
	RegYmm lhs;
	RegYmm rhs;
};

struct VmulpsYmmYmmYmm {
	RegYmm destination;
	RegYmm lhs;
	RegYmm rhs;
};

struct VdivpsYmmYmmYmm {
	RegYmm destination;
	RegYmm lhs;
	RegYmm rhs;
};

using Instruction = std::variant<
	Ret,
	Push64,
	Pop64,
	JmpLbl,
	XorR64R64,
	AndR8Imm,
	AddR64Imm,
	SubR64Imm,
	Inc64,
	CmpR64R64,
	MovR64R64,
	VbroadcastssLbl,
	VmovapsYmmYmm,
	VmovapsYmmMem,
	VmovapsMemYmm,
	VaddpsYmmYmmYmm,
	VsubpsYmmYmmYmm,
	VmulpsYmmYmmYmm,
	VdivpsYmmYmmYmm
>;

struct LabeledInstruction {
	InstructionLabel label;
	Instruction instruction;
};