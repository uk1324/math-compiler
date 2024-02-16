#include "codeGenerator.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"
#include "floatingPoint.hpp"
#include <algorithm>

CodeGenerator::CodeGenerator() {
	initialize(std::span<const FunctionParameter>(), std::span<const FunctionInfo>());
}

void CodeGenerator::initialize(std::span<const FunctionParameter> parameters, std::span<const FunctionInfo> functions) {
	registerToLastUsage.clear();
	virtualRegisterToLocation.clear();
	for (i64 i = 0; i < i64(std::size(registerAllocations)); i++) {
		registerAllocations[i] = std::nullopt;
	}
	currentInstructionIndex = 0;
	this->parameters = parameters;
	stackMemoryAllocated = 0;
	stackAllocations.clear();
	this->functions = functions;
	a.reset();
}

// TODO: Encoding R12 is different.
const auto inputArrayRegister = Reg64::RDI;
const auto inputArrayRegisterArgumentIndex = 0;
const auto outputArrayRegister = Reg64::R13;
const auto outputArrayRegisterArgumentIndex = 1;
const auto arraySizeRegister = Reg64::R14;
const auto arraySizeRegisterArgumentIndex = 2;
const auto indexRegister = Reg64::R15;

MachineCode CodeGenerator::compile(
	const std::vector<IrOp>& irCode,
	std::span<const FunctionInfo> functions,
	std::span<const FunctionParameter> parameters) {
	initialize(parameters, functions);
	computeRegisterLastUsage(irCode);

	a.xor_(indexRegister, indexRegister);

	const auto conditionCheckLabel = a.allocateLabel();
	a.jmp(conditionCheckLabel);

	const auto loopStartLabel = a.allocateLabel();
	a.setLabelOnNextInstruction(loopStartLabel);

	for (i64 i = 0; i < i64(irCode.size()); i++) {
		const auto& op = irCode[i];
		currentInstructionIndex = i;
		std::visit(overloaded{
			[&](const LoadConstantOp& op) { loadConstantOp(op); },
			[&](const LoadVariableOp& op) { generate(op); },
			[&](const AddOp& op) { addOp(op); },
			[&](const SubtractOp& op) { subtractOp(op); },
			[&](const MultiplyOp& op) { multiplyOp(op); },
			[&](const DivideOp& op) { divideOp(op); },
			[&](const XorOp& op) { generate(op); },
			[&](const NegateOp& op) { generate(op); },
			[&](const FunctionOp& op) { generate(op); },
			[&](const ReturnOp& op) { returnOp(op); }
		}, op);
	}

	a.add(inputArrayRegister, u32(parameters.size() * YMM_REGISTER_SIZE));
	a.add(outputArrayRegister, YMM_REGISTER_SIZE);
	a.inc(indexRegister);

	a.setLabelOnNextInstruction(conditionCheckLabel);

	a.cmp(indexRegister, arraySizeRegister);
	a.jl(loopStartLabel);

	emitPrologueAndEpilogue();

	MachineCode machineCode;
	machineCode.generateFrom(a);

	return machineCode;
}

void CodeGenerator::computeRegisterLastUsage(const std::vector<IrOp>& irCode) {
	for (i64 i = 0; i < i64(irCode.size()); i++) {
		auto add = [this, &i](Register reg) {
			registerToLastUsage[reg] = i;
		};

		// TODO: Could make a function that takes a function and executes this input function on all of the registers of the irOp.
		// TODO: Should the operand be added to the last usage?
		auto& op = irCode[i];
		std::visit(overloaded{
			[&](const LoadConstantOp& op) {
				add(op.destination);
			},
			[&](const LoadVariableOp& op) {
				add(op.destination);
			},
			[&](const AddOp& op) {
				add(op.destination);
				add(op.lhs);
				add(op.rhs);
			},
			[&](const SubtractOp& op) {
				add(op.destination);
				add(op.lhs);
				add(op.rhs);
			},
			[&](const MultiplyOp& op) {
				add(op.destination);
				add(op.lhs);
				add(op.rhs);
			},
			[&](const DivideOp& op) {
				add(op.destination);
				add(op.lhs);
				add(op.rhs);
			},
			[&](const XorOp& op) {
				add(op.destination);
				add(op.lhs);
				add(op.rhs);
			},
			[&](const NegateOp& op) {
				add(op.destination);
				add(op.operand);
			},
			[&](const FunctionOp& op) {
				add(op.destination);
				for (const auto& argument : op.arguments) {
					add(argument);
				}
			},
			[&](const ReturnOp& op) {
				add(op.returnedRegister);
			}
		}, op);
	}
}

void CodeGenerator::computeRegisterFirstAssigned(const std::vector<IrOp>& irCode) {
	for (i64 i = 0; i < i64(irCode.size()); i++) {
		callWithOutputRegisters(
			irCode[i],
			[this, &i](Register reg) {
				// insert() only inserts if the key is not set yet.
				registerToFirstAssigned.insert({ reg, i });
			}
		);
	}
}

void CodeGenerator::movToYmmFromMemoryLocation(RegYmm destination, const MemoryLocation& memoryLocation) {
	std::visit(overloaded{
		[&](const ConstantLocation& location) {
			const auto label = a.allocateData(location.value);
			a.vbroadcastss(destination, label);
		},
		[&](const RegisterConstantOffsetLocation& location) {
			a.vmovaps(destination, location.registerWithAddress, u32(location.offset));
		}
	}, memoryLocation);
}

void CodeGenerator::movToYmmFromYmm(RegYmm destination, RegYmm source) {
	if (destination == source) {
		return;
	}
	a.vmovaps(destination, source);
}

void CodeGenerator::emitPrologueAndEpilogue() {
	const auto codeSizeAtStart = a.instructions.size();
	auto offset = [&]() {
		return a.instructions.size() - codeSizeAtStart;
	};

	// All of the YMM registers are caller saved so they don't need to be saved.
	// Windows:
	// https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170
	// When present, the upper portions of YMM0-YMM15 and ZMM0-ZMM15 are also volatile.
	// System V ABI:
	// Figure 3.4: Register Usage
	// All XMM register which also includes the YMM register are caller saved.

	const auto maxPossibleIncreaseCausedByAligning = 32;

	auto stackMemoryAllocatedTotal = maxPossibleIncreaseCausedByAligning + stackMemoryAllocated + SHADOW_SPACE_SIZE * 8;

	//const Reg64 registerToSave[] = {
	//	inputArrayRegister,
	//	outputArrayRegister,
	//	arraySizeRegister,
	//	indexRegister
	//};
	i64 pushedMemory = 8;
	auto push = [&](Reg64 reg) {
		pushedMemory += 8;
		a.push(reg, offset());
	};

	/*if (stackMemoryAllocatedTotal > 0)*/ {
		push(Reg64::RBP);

		push(inputArrayRegister);
		a.mov(inputArrayRegister, INTEGER_FUNCTION_INPUT_REGISTERS[inputArrayRegisterArgumentIndex], offset());

		push(outputArrayRegister);
		//a.push(outputArrayRegister, offset());
		a.mov(outputArrayRegister, INTEGER_FUNCTION_INPUT_REGISTERS[outputArrayRegisterArgumentIndex], offset());

		push(arraySizeRegister);
		//a.push(arraySizeRegister, offset());
		a.mov(arraySizeRegister, INTEGER_FUNCTION_INPUT_REGISTERS[arraySizeRegisterArgumentIndex], offset());

		//a.push(indexRegister, offset());
		push(indexRegister);

		const auto requiredAlignment = 16;
		const auto misalignment = pushedMemory % requiredAlignment;
		stackMemoryAllocatedTotal += misalignment == 0 ? 0 : requiredAlignment - misalignment;

		a.mov(Reg64::RBP, Reg64::RSP, offset());
		a.and_(Reg8::BPL, 0b11100000, offset());

		a.sub(Reg64::RSP, u32(stackMemoryAllocatedTotal), offset());
	}

	/*if (stackMemoryAllocated > 0)*/ {
		a.add(Reg64::RSP, u32(stackMemoryAllocatedTotal));
		a.pop(indexRegister);
		a.pop(arraySizeRegister);
		a.pop(outputArrayRegister);
		a.pop(inputArrayRegister);
		a.pop(Reg64::RBP);
	}
	a.ret();
}

RegYmm CodeGenerator::getRegisterLocation(Register reg, std::span<const Register> virtualRegistersThatCanNotBeSpilled) {
	auto& location = virtualRegisterToLocation[reg];

	if (location.registerLocation.has_value()) {
		return *location.registerLocation;
	}

	const auto actualRegister = allocateRegister(virtualRegistersThatCanNotBeSpilled);
	registerAllocations[regIndex(actualRegister)] = reg;
	location.registerLocation = actualRegister;
	if (location.memoryLocation.has_value()) {
		movToYmmFromMemoryLocation(actualRegister, *location.memoryLocation);
		return actualRegister;
	}

	// if there is no memory location and no register location then it is (should be) a newly created virtual register with no value assigned to yet.

	return actualRegister;
}

RegYmm CodeGenerator::allocateRegister(std::span<const Register> virtualRegistersThatCanNotBeSpilled) {
	Register virtualRegisterThatIsNotUsedForLongestFromNow;
	RegYmm virtualRegisterThatIsNotUsedForLongestFromNowRegisterLocation;
	i64 maxDistance = -1;

	for (u8 actualRegisterIndex = 0; actualRegisterIndex < i64(std::size(registerAllocations)); actualRegisterIndex++) {
		const auto actualRegister = regYmmFromIndex(actualRegisterIndex);
		const auto optVirtualRegister = registerAllocations[actualRegisterIndex];

		if (!optVirtualRegister.has_value()) {
			return actualRegister;
		}
		const auto virtualRegister = *optVirtualRegister;

		const auto lastUsage = registerToLastUsage[virtualRegister];
		if (std::ranges::contains(virtualRegistersThatCanNotBeSpilled, virtualRegister)) {
			continue;
		}

		if (lastUsage < currentInstructionIndex) {
			return actualRegister;
		}

		const auto distance = lastUsage - currentInstructionIndex;
		if (distance > maxDistance) {
			maxDistance = distance;
			virtualRegisterThatIsNotUsedForLongestFromNow = virtualRegister;
			virtualRegisterThatIsNotUsedForLongestFromNowRegisterLocation = actualRegister;
		}
	}

	const auto& virtualRegisterToSpill = virtualRegisterThatIsNotUsedForLongestFromNow;
	const auto& virtualRegisterToSpillRegisterLocation = virtualRegisterThatIsNotUsedForLongestFromNowRegisterLocation;
	auto& virtualRegisterToSpillLocation = virtualRegisterToLocation[virtualRegisterToSpill];

	virtualRegisterToSpillLocation.registerLocation = std::nullopt;

	if (virtualRegisterToSpillLocation.memoryLocation.has_value()) {
		return virtualRegisterToSpillRegisterLocation;
	}

	const auto baseOffset = stackAllocate(YMM_REGISTER_SIZE, YMM_REGISTER_ALIGNMENT);
	virtualRegisterToSpillLocation.memoryLocation = baseOffset.location();
	a.vmovaps(STACK_BASE_REGISTER, baseOffset.baseOffset, virtualRegisterToSpillRegisterLocation);
	return virtualRegisterToSpillRegisterLocation;
}

void CodeGenerator::loadConstantOp(const LoadConstantOp& op) {
	const auto destination = getRegisterLocation(op.destination);
	//loadRegYmmConstant32(destination, op.constant);
	const auto label = a.allocateData(op.constant);
	a.vbroadcastss(destination, label);
	virtualRegisterToLocation[op.destination].memoryLocation = ConstantLocation{
		.value = op.constant
	};
	// TODO: if equal to 0 then xor
}

void CodeGenerator::generate(const LoadVariableOp& op) {
	const auto destination = getRegisterLocation(op.destination);

	auto& location = virtualRegisterToLocation[op.destination];
	const auto offset = op.variableIndex * YMM_REGISTER_SIZE;
	location.memoryLocation = RegisterConstantOffsetLocation{
		.registerWithAddress = inputArrayRegister,
		.offset = offset,
	};
	location.registerLocation = destination;
	movToYmmFromMemoryLocation(destination, *location.memoryLocation);

	/*for (Register virtualRegister = 0; virtualRegister < parameters.size(); virtualRegister++) {
		auto& location = virtualRegisterToLocation[virtualRegister];
		const auto offset = virtualRegister * YMM_REGISTER_SIZE;
		location.memoryLocation = RegisterConstantOffsetLocation{
			.registerWithAddress = inputArrayRegister,
			.offset = offset,
		};
	}*/
}

void CodeGenerator::addOp(const AddOp& op) {
	const Register reserved[] = { op.lhs, op.rhs, op.destination };
	const auto destination = getRegisterLocation(op.destination, reserved);
	const auto lhs = getRegisterLocation(op.lhs, reserved);
	const auto rhs = getRegisterLocation(op.rhs, reserved);
	a.vaddps(destination, lhs, rhs);
}

void CodeGenerator::subtractOp(const SubtractOp& op) {
	const Register reserved[] = { op.lhs, op.rhs, op.destination };
	const auto destination = getRegisterLocation(op.destination, reserved);
	const auto lhs = getRegisterLocation(op.lhs, reserved);
	const auto rhs = getRegisterLocation(op.rhs, reserved);
	a.vsubps(destination, lhs, rhs);
}

void CodeGenerator::multiplyOp(const MultiplyOp& op) {
	const Register reserved[] = { op.lhs, op.rhs, op.destination };
	const auto destination = getRegisterLocation(op.destination, reserved);
	const auto lhs = getRegisterLocation(op.lhs, reserved);
	const auto rhs = getRegisterLocation(op.rhs, reserved);
	a.vmulps(destination, lhs, rhs);
}

void CodeGenerator::divideOp(const DivideOp& op) {
	const Register reserved[] = { op.lhs, op.rhs, op.destination };
	const auto destination = getRegisterLocation(op.destination, reserved);
	const auto lhs = getRegisterLocation(op.lhs, reserved);
	const auto rhs = getRegisterLocation(op.rhs, reserved);
	a.vdivps(destination, lhs, rhs);
}

void CodeGenerator::generate(const XorOp& op) {
	const Register reserved[] = { op.lhs, op.rhs, op.destination };
	const auto destination = getRegisterLocation(op.destination, reserved);
	const auto lhs = getRegisterLocation(op.lhs, reserved);
	const auto rhs = getRegisterLocation(op.rhs, reserved);
	// There are multitple instructions that perform xor on ymm register not sure which one to use. xorps, xorpd, pxor
	a.vxorps(destination, lhs, rhs);
}

void CodeGenerator::generate(const NegateOp& op) {
	const Register reserved[] = { op.operand, op.destination };
	const auto destination = getRegisterLocation(op.destination, reserved);
	const auto operand = getRegisterLocation(op.operand, reserved);
	const auto dataLabel = a.allocateData(std::bit_cast<float>(F32_SIGN_MASK));
	a.vbroadcastss(destination, dataLabel);
	a.vxorps(destination, destination, operand);
}

void CodeGenerator::generate(const FunctionOp& op) {
	// All YMM registers are caller saved.
	for (i64 realRegisterIndex = 0; realRegisterIndex < YMM_REGISTER_COUNT; realRegisterIndex++) {
		const auto virtualRegisterOpt = registerAllocations[realRegisterIndex];
		if (!virtualRegisterOpt.has_value()) {
			continue;
		}

		const auto virtualRegister = *virtualRegisterOpt;
		const auto locationIt = virtualRegisterToLocation.find(virtualRegister);
		if (locationIt == virtualRegisterToLocation.end()) {
			ASSERT_NOT_REACHED();
			continue;
		}

		auto& location = locationIt->second;
		if (location.memoryLocation.has_value()) {
			continue;
		}

		const auto realRegister = RegYmm(realRegisterIndex);
		const auto memory = stackAllocate(YMM_REGISTER_SIZE, YMM_REGISTER_ALIGNMENT);
		location.memoryLocation = memory.location();
		a.vmovaps(STACK_BASE_REGISTER, memory.baseOffset, realRegister);
	}

	for (u8 i = 0; i < op.arguments.size(); i++) {
		const auto virtualRegister = op.arguments[i];
		const auto locationIt = virtualRegisterToLocation.find(virtualRegister);
		if (locationIt == virtualRegisterToLocation.end()) {
			ASSERT_NOT_REACHED();
			continue;
		}
		const auto& location = locationIt->second;
		const auto vectorCallPassByValueInRegister = i < 6;

		if (location.registerLocation.has_value()) {
			if (vectorCallPassByValueInRegister) {
				movToYmmFromYmm(regYmmFromIndex(i), *location.registerLocation);
			} else {
				ASSERT_NOT_REACHED();
			}
		} else if (location.memoryLocation.has_value()) {
			if (vectorCallPassByValueInRegister) {
				movToYmmFromMemoryLocation(regYmmFromIndex(i), *location.memoryLocation);
			} else {
				ASSERT_NOT_REACHED();
			}
		} else {
			ASSERT_NOT_REACHED();
			continue;
		}
	}

	for (i64 realRegisterIndex = 0; realRegisterIndex < YMM_REGISTER_COUNT; realRegisterIndex++) {
		registerAllocations[realRegisterIndex] = std::nullopt;
	}

	for (auto& [_, location] : virtualRegisterToLocation) {
		location.registerLocation = std::nullopt;
	}

	const auto functionInfo = std::ranges::find_if(functions, [&](const FunctionInfo& f) { return f.name == op.functionName; });
	if (functionInfo == functions.end()) {
		ASSERT_NOT_REACHED();
		return;
	}
	// Can't use RIP relative jumps because they take 32 bit signed operands. I tried and the OS allocates memory that is more than 2^31 bytes away from the other function pointers.
	a.mov(Reg64::R9, std::bit_cast<u64>(functionInfo->address));
	a.call(Reg64::R9);

	const auto destination = getRegisterLocation(op.destination);
	const auto VECTORCALL_RETURN_REGISTER_0 = RegYmm::YMM0;
	movToYmmFromYmm(destination, VECTORCALL_RETURN_REGISTER_0);

	auto& location = virtualRegisterToLocation[op.destination];
	registerAllocations[regIndex(VECTORCALL_RETURN_REGISTER_0)] = op.destination;
	location.registerLocation = VECTORCALL_RETURN_REGISTER_0;
}

void CodeGenerator::returnOp(const ReturnOp& op) {
	const auto source = getRegisterLocation(op.returnedRegister);
	a.vmovaps(outputArrayRegister, 0, source);
}

CodeGenerator::BaseOffset CodeGenerator::stackAllocate(i32 size, i32 aligment) {
	stackMemoryAllocated += size;
	const BaseOffset result{ .baseOffset = -stackMemoryAllocated };

	return result;
}

CodeGenerator::RegisterConstantOffsetLocation CodeGenerator::BaseOffset::location() const {
	return RegisterConstantOffsetLocation{
		.registerWithAddress = CodeGenerator::STACK_BASE_REGISTER,
		.offset = baseOffset,
	};
}
