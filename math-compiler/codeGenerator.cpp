#include "codeGenerator.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"

CodeGenerator::CodeGenerator() {
	initialize();
}

void CodeGenerator::initialize() {
	code.clear();
	data.clear();
	ripRelativeImmediate32Allocations.clear();
	stackAllocations.clear();
	stackMemoryAllocated = 0;
	registerLocations.clear();
}

const std::vector<u8>& CodeGenerator::compile(const std::vector<IrOp>& irCode) {
	initialize();

	//emitAddssRegXmmRegXmm(ModRMRegisterXmm::XMM0, ModRMRegisterXmm::XMM5);

	//emitPushReg64(ModRMRegisterX64::RBP);
	//emitVmovapsToRegYmmFromRegYmm(ModRMRegisterXmm::XMM1, ModRMRegisterXmm::XMM15);
	/*emitVmovapsToMemReg32DispI32FromRegYmm(ModRMRegisterX86::CX, 5, ModRMRegisterXmm::XMM5);
	emitVmovapsToRegYmmFromMemReg32DispI32(ModRMRegisterXmm::XMM5, ModRMRegisterX86::CX, 5);*/
	emitVaddpsRegYmmRegYmmRegYmm(ModRMRegisterXmm::XMM5, ModRMRegisterXmm::XMM6, ModRMRegisterXmm::XMM7);
	emitVsubpsRegYmmRegYmmRegYmm(ModRMRegisterXmm::XMM6, ModRMRegisterXmm::XMM7, ModRMRegisterXmm::XMM8);
	emitVmulpsRegYmmRegYmmRegYmm(ModRMRegisterXmm::XMM7, ModRMRegisterXmm::XMM8, ModRMRegisterXmm::XMM9);
	emitVdivpsRegYmmRegYmmRegYmm(ModRMRegisterXmm::XMM8, ModRMRegisterXmm::XMM9, ModRMRegisterXmm::XMM10);
	/*for (const auto& op : irCode) {
		std::visit(overloaded{
			[&](const LoadConstantOp& op) { loadConstantOp(op); },
			[&](const AddOp& op) { addOp(op); },
			[&](const MultiplyOp& op) { multiplyOp(op); },
			[&](const ReturnOp& op) {
				const auto location = getRegisterLocation(op.returnedRegister);
				emitMovssToRegXmmFromMemReg32DispI32(
					ModRMRegisterXmm::XMM0,
					ModRMRegisterX86::BP,
					location.location.baseOffset);
				emitPopReg64(ModRMRegisterX64::RBP);
				emitRet();
			}
		}, op);
	}*/

	emitRet();
	return code;
}

void CodeGenerator::patchExecutableCodeRipAddresses(u8* code, const u8* data) const {
	for (const auto& allocation : ripRelativeImmediate32Allocations) {
		const auto ripOffsetAddress = code + allocation.ripOffsetBytesCodeIndex;
		const auto dataOffsetAddress = data + allocation.immediateDataSectionOffset;
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

void CodeGenerator::loadConstantOp(const LoadConstantOp& op) {
	//emitMovssToMemReg32DispI32FromRegXmm()
	/*const auto baseOffset = stackAllocate(sizeof(op.constant), alignof(op.constant));*/
	const auto location = getRegisterLocation(op.destination);
	movssToRegXmmConstant32(ModRMRegisterXmm::XMM0, op.constant);
	movssToRegisterLocationFromRegXmm(location, ModRMRegisterXmm::XMM0);
}

void CodeGenerator::addOp(const AddOp& op) {
	const auto lhs = getRegisterLocation(op.lhs);
	const auto rhs = getRegisterLocation(op.rhs);
	movssToRegXmmFromRegisterLocation(ModRMRegisterXmm::XMM0, lhs);
	movssToRegXmmFromRegisterLocation(ModRMRegisterXmm::XMM1, rhs);
	emitAddssRegXmmRegXmm(ModRMRegisterXmm::XMM0, ModRMRegisterXmm::XMM1);
	const auto destination = getRegisterLocation(op.destination);
	movssToRegisterLocationFromRegXmm(destination, ModRMRegisterXmm::XMM0);
}

void CodeGenerator::multiplyOp(const MultiplyOp& op) {
	const auto lhs = getRegisterLocation(op.lhs);
	const auto rhs = getRegisterLocation(op.rhs);
	movssToRegXmmFromRegisterLocation(ModRMRegisterXmm::XMM0, lhs);
	movssToRegXmmFromRegisterLocation(ModRMRegisterXmm::XMM1, rhs);
	emitMulssRegXmmRegXmm(ModRMRegisterXmm::XMM0, ModRMRegisterXmm::XMM1);
	const auto destination = getRegisterLocation(op.destination);
	movssToRegisterLocationFromRegXmm(destination, ModRMRegisterXmm::XMM0);
}

void CodeGenerator::emitU8(u8 value) {
	code.push_back(value);
}

void CodeGenerator::emitI8(i8 value) {
	emitU8(std::bit_cast<u8>(value));
}

void CodeGenerator::emitU32(u32 value) {
	const auto bytes = reinterpret_cast<u8*>(&value);
	emitU8(bytes[0]);
	emitU8(bytes[1]);
	emitU8(bytes[2]);
	emitU8(bytes[3]);
}

void CodeGenerator::emitI32(u32 value) {
	emitU32(std::bit_cast<i32>(value));
}

void CodeGenerator::emitRet() {
	emitU8(0xC3);
}

void CodeGenerator::dataEmitU8(u8 value) {
	data.push_back(value);
}

void CodeGenerator::dataEmitU32(u32 value) {
	const auto bytes = reinterpret_cast<u8*>(&value);
	dataEmitU8(bytes[0]);
	dataEmitU8(bytes[1]);
	dataEmitU8(bytes[2]);
	dataEmitU8(bytes[3]);
}

u8 CodeGenerator::encodeModRmByte(u8 mod, u8 reg, u8 rm) {
	return (mod << 6) | (reg << 3) | rm;
}

u8 CodeGenerator::encodeModRmReg32DispI8Reg32(ModRMRegisterX86 registerWithAddress, ModRMRegisterX86 reg) {
	return encodeModRmByte(0b01, static_cast<u8>(reg), static_cast<u8>(registerWithAddress));
}

u8 CodeGenerator::encodeModRmReg32DispI32Reg32(ModRMRegisterX86 registerWithAddress, ModRMRegisterX86 reg) {
	return encodeModRmByte(0b10, static_cast<u8>(reg), static_cast<u8>(registerWithAddress));
}

//u8 CodeGenerator::encodeModRmReg32Disp8(ModRMRegisterX86 registerWithAddress, u8 displacement, ModRMRegisterX86 reg) {
//	return encodeModRmByte(0b01, static_cast<u8>(reg), static_cast<u8>(registerWithAddress));
//}

//u8 CodeGenerator::encodeModRmReg32Disp8(ModRMRegisterX86 baseRegister, u8 displacement) {
//	return encodeModRmByte(0b01, static_cast<u8>(baseRegister), )
//}

u8 CodeGenerator::encodeModRmDirectAddressingByte(u8 g, u8 e) {
	return encodeModRmByte(0b11, g, e);
}

u8 CodeGenerator::encodeModRmDirectAddressingByte(ModRMRegisterXmm g, ModRMRegisterXmm e) {
	ASSERT(static_cast<u8>(g) <= 7); 
	ASSERT(static_cast<u8>(e) <= 7);
	return encodeModRmByte(0b11, static_cast<u8>(g), static_cast<u8>(e));
}

u8 CodeGenerator::encodeModRmDirectAddressingByte(ModRMRegisterX86 g, ModRMRegisterX86 e) {
	ASSERT(static_cast<u8>(g) <= 7);
	ASSERT(static_cast<u8>(e) <= 7);
	return encodeModRmByte(0b11, static_cast<u8>(g), static_cast<u8>(e));
}

// Maybe make constexpr and store often used constants
u8 CodeGenerator::encodeRexPrefixByte(bool w, bool r, bool x, bool b) {
	// w - use 64 bit operand size else use default operand size (mostly 32 bit but can differ)
	// r - MODRM.reg extension bit
	// x - SIB.index extension bit
	// b - MODRM.rm field or the SIB.base field extension bit
	return 0b0100'0000 | (w << 3) | (r << 2) | (x << 1) | static_cast<u8>(b);
}

void CodeGenerator::emitModRmReg32DispI32Reg32(ModRMRegisterX86 registerWithAddress, i32 displacement, ModRMRegisterX86 reg) {
	ASSERT(static_cast<u8>(reg) <= 7);
	emitU8(encodeModRmReg32DispI32Reg32(registerWithAddress, static_cast<ModRMRegisterX86>(reg)));
	emitI32(displacement);
}

static constexpr u8 ADD_REG_REG_OP_CODE_BYTE = 0x01;

void CodeGenerator::emitAddRegisterToRegister32(ModRMRegisterX86 rhs, ModRMRegisterX86 lhs) {
	emitU8(ADD_REG_REG_OP_CODE_BYTE);
	emitU8(encodeModRmDirectAddressingByte(lhs, rhs));
}

template<typename T>
T takeFirst3Bits(T r) {
	return static_cast<T>(0b111 & static_cast<u8>(r));

}

template<typename T>
bool take4thBit(T r) {
	// Assumes bits after the fourth one are set to zero.
	return static_cast<u8>(r) >> 3;
};

void CodeGenerator::emitAddRegisterToRegister64(ModRMRegisterX64 rhs, ModRMRegisterX64 lhs) {
	
	emitU8(encodeRexPrefixByte(true, take4thBit(rhs), false, take4thBit(lhs)));
	emitU8(ADD_REG_REG_OP_CODE_BYTE);
	emitU8(encodeModRmDirectAddressingByte(
		static_cast<ModRMRegisterX86>(takeFirst3Bits(lhs)), 
		static_cast<ModRMRegisterX86>(takeFirst3Bits(rhs))
	));
}

void CodeGenerator::emitMovssRegToRegXmm(ModRMRegisterXmm destination, ModRMRegisterXmm source) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x10);
	emitU8(encodeModRmDirectAddressingByte(destination, source));
}

void CodeGenerator::emitAddssRegXmmRegXmm(ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x58);
	emitU8(encodeModRmDirectAddressingByte(lhs, rhs));
}

void CodeGenerator::emitMulssRegXmmRegXmm(ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x59);
	emitU8(encodeModRmDirectAddressingByte(lhs, rhs));
}

void CodeGenerator::emitMovssToRegXmmFromMemReg32DispI8(ModRMRegisterXmm destination, ModRMRegisterX86 regWithSourceAddress, i8 addressDisplacement) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x10);
	emitU8(encodeModRmReg32DispI8Reg32(regWithSourceAddress, static_cast<ModRMRegisterX86>(destination)));
	emitU8(addressDisplacement);
}

void CodeGenerator::emitMovssToRegXmmFromMemReg32DispI32(ModRMRegisterXmm destination, ModRMRegisterX86 regWithSourceAddress, i32 addressDisplacement) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x10);
	emitU8(encodeModRmReg32DispI32Reg32(regWithSourceAddress, static_cast<ModRMRegisterX86>(destination)));
	emitI32(addressDisplacement);
}

void CodeGenerator::emitMovssToMemReg32DispI8FromRegXmm(ModRMRegisterX86 regWithDestinationAddress, i8 addressDisplacement, ModRMRegisterXmm source) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x11);
	emitU8(encodeModRmReg32DispI8Reg32(regWithDestinationAddress, static_cast<ModRMRegisterX86>(source)));
	emitI8(addressDisplacement);
}

void CodeGenerator::emitMovssToMemReg32DispI32FromRegXmm(ModRMRegisterX86 regWithDestinationAddress, i32 addressDisplacement, ModRMRegisterXmm source) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x11);
	emitU8(encodeModRmReg32DispI32Reg32(regWithDestinationAddress, static_cast<ModRMRegisterX86>(source)));
	emitI32(addressDisplacement);
}

void CodeGenerator::movssToRegXmmConstant32(ModRMRegisterXmm destination, float immediate) {
	emitU8(0xF3);
	emitU8(0x0F);
	emitU8(0x10);
	// RIP relative addressing.
	const u8 mod = 0b00, reg = static_cast<u8>(destination), rm = 0b101;
	emitU8(encodeModRmByte(mod, reg, rm));
	const i64 ripOffsetBytesCodeOffset = code.size(); // Points to last by of the current code. Which is where the next bytes begin.
	emitI32(0);

	const i64 dataSectionOffset = data.size();
	dataEmitU32(std::bit_cast<u32>(immediate));
	
	ripRelativeImmediate32Allocations.push_back(RipRelativeImmediate32Allocation{
		.value = immediate,
		.ripOffsetBytesCodeIndex = ripOffsetBytesCodeOffset,
		.immediateDataSectionOffset = dataSectionOffset,
	});
}

void CodeGenerator::emitPushReg64(ModRMRegisterX64 reg) {
	emitU8(0x50 + static_cast<u8>(reg));
}

void CodeGenerator::emitPopReg64(ModRMRegisterX64 reg) {
	emitU8(0x58 + static_cast<u8>(reg));
}

static bool isInBitRange(u8 value, u8 allowedBitCount) {
	return value < (1 << allowedBitCount);
}

void CodeGenerator::emit2ByteVex(bool r, u8 vvvv, bool l, u8 pp) {
	ASSERT(isInBitRange(vvvv, 4));
	ASSERT(isInBitRange(pp, 2));

	emitU8(0xC5);
	emitU8(
		(u8(r) << 7) |
		(vvvv << 3) |
		(u8(l) << 2) |
		u8(pp));
}

void CodeGenerator::emit3ByteVex(bool r, bool x, bool b, u8 m_mmmm, bool w, u8 vvvv, bool l, u8 pp) {
	ASSERT(isInBitRange(m_mmmm, 5));
	ASSERT(isInBitRange(vvvv, 4));
	ASSERT(isInBitRange(pp, 2));

	emitU8(0xC4);
	emitU8(
		(u8(r) << 7) |
		(u8(x) << 6) |
		(u8(b) << 5) |
		m_mmmm);
	emitU8(
		(u8(w) << 7) |
		(vvvv << 3) |
		(u8(l) << 2) |
		u8(pp));
}

//void CodeGenerator::emitRegYmmRegYmmVexByte(ModRMRegisterXmm a, ModRMRegisterXmm b) {
//	/*const auto destination4thBit = take4thBit(a);
//	const auto source4thBit = take4thBit(b);
//	if (source4thBit == 0) {
//		emit2ByteVex(!source4thBit, 0b1111, 1, 0b00);
//	}
//	else {
//		emit3ByteVex(!source4thBit, 0, !destination4thBit, 0b000001, 0, 0b1111, 1, 0b00);
//	}*/
//}

void CodeGenerator::emitInstructionRegYmmRegYmmRegYmm(u8 opCode, ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	const auto destination4thBit = take4thBit(destination);
	const auto rhs4thBit = take4thBit(rhs);

	const auto invertedLhs = ~static_cast<u8>(lhs) & 0b1111;
	//emit2ByteVex(1, invertedLhs, 1, 0b00);
	if (rhs4thBit == 0) {
		emit2ByteVex(!destination4thBit, invertedLhs, 1, 0b00);
	}
	else {
		emit3ByteVex(!destination4thBit, 0, !rhs4thBit, 0b000001, 0, invertedLhs, 1, 0b00);
	}
	emitU8(opCode);
	emitU8(encodeModRmDirectAddressingByte(takeFirst3Bits(destination), takeFirst3Bits(rhs)));
}

void CodeGenerator::emitVmovapsToRegYmmFromRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm source) {
	const auto destination4thBit = take4thBit(destination);
	const auto source4thBit = take4thBit(source);
	if (source4thBit == 0) {
		emit2ByteVex(!source4thBit, 0b1111, 1, 0b00);
	} else {
		emit3ByteVex(!source4thBit, 0, !destination4thBit, 0b000001, 0, 0b1111, 1, 0b00);
	}
	emitU8(0x29);
	emitU8(encodeModRmDirectAddressingByte(takeFirst3Bits(source), takeFirst3Bits(destination)));
}

void CodeGenerator::emitVmovapsToMemReg32DispI32FromRegYmm(ModRMRegisterX86 regWithDestinationAddress, i32 addressDisplacement, ModRMRegisterXmm source) {
	emit2ByteVex(!take4thBit(source), 0b1111, 1, 0b00);
	emitU8(0x29);
	emitModRmReg32DispI32Reg32(
		regWithDestinationAddress, 
		addressDisplacement, 
		static_cast<ModRMRegisterX86>(takeFirst3Bits(source)));
}

void CodeGenerator::emitVmovapsToRegYmmFromMemReg32DispI32(ModRMRegisterXmm destination, ModRMRegisterX86 regWithSourceAddress, i32 addressDisplacement)  {
	emit2ByteVex(!take4thBit(destination), 0b1111, 1, 0b00);
	emitU8(0x28);
	emitModRmReg32DispI32Reg32(
		regWithSourceAddress, 
		addressDisplacement, 
		static_cast<ModRMRegisterX86>(takeFirst3Bits(destination)));
}

void CodeGenerator::emitVaddpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	emitInstructionRegYmmRegYmmRegYmm(0x58, destination, lhs, rhs);
}

void CodeGenerator::emitVsubpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	emitInstructionRegYmmRegYmmRegYmm(0x5C, destination, lhs, rhs);
}

void CodeGenerator::emitVmulpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	emitInstructionRegYmmRegYmmRegYmm(0x59, destination, lhs, rhs);
}

void CodeGenerator::emitVdivpsRegYmmRegYmmRegYmm(ModRMRegisterXmm destination, ModRMRegisterXmm lhs, ModRMRegisterXmm rhs) {
	emitInstructionRegYmmRegYmmRegYmm(0x5E, destination, lhs, rhs);
}

CodeGenerator::BaseOffset CodeGenerator::stackAllocate(i32 size, i32 aligment) {
	const BaseOffset result{ .baseOffset = -stackMemoryAllocated };
	stackMemoryAllocated += size;

	return result;
}

CodeGenerator::RegisterLocation CodeGenerator::getRegisterLocation(Register reg) {
	const auto location = registerLocations.find(reg);
	if (location != registerLocations.end()) {
		return location->second;
	}

	const auto baseOffset = stackAllocate(4, 4);
	RegisterLocation result{
		.location = baseOffset
	};
	registerLocations.insert({ reg, result });
	return result;
}

void CodeGenerator::movssToRegisterLocationFromRegXmm(const RegisterLocation& to, ModRMRegisterXmm from) {
	emitMovssToMemReg32DispI32FromRegXmm(ModRMRegisterX86::BP, to.location.baseOffset, from);
}

void CodeGenerator::movssToRegXmmFromRegisterLocation(ModRMRegisterXmm to, const RegisterLocation& from) {
	emitMovssToRegXmmFromMemReg32DispI32(to, ModRMRegisterX86::BP, from.location.baseOffset);
}

//void CodeGenerator::emitMovsMemReg32Disp8ToRegXmm(ModRMRegisterX86 regWithAddress, u8 displacement, ModRMRegisterXmm source) {
//	
//}

//void CodeGenerator::emitMovsMemRegDisp8ToReg(u8 ebpDisplacement, ModRMRegisterXmm source) {
//}

//void CodeGenerator::emitMovsMemEbp8ToReg(u8 ebpDisplacement, ModRMRegisterXmm source) {
//	emitU8(0xF3);
//	emitU8(0x0F);
//	emitU8(0x10);
//	emitU8(encodeModRmByte(0b01, static_cast<u8>(ModRMRegisterXmm::XMM0), 0b101));
//	emitU8(ebpDisplacement);
//}
//
//void CodeGenerator::emitMovsMemEbp32ToReg(u32 ebpDisplacement, ModRMRegisterXmm source) {
//	emitU8(0xF3);
//	emitU8(0x0F);
//	emitU8(0x10);
//	emitU8(encodeModRmByte(0b10, static_cast<u8>(ModRMRegisterXmm::XMM0), 0b101));
//	emitU8(ebpDisplacement);
//}
