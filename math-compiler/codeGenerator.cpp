#include "codeGenerator.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"
#include "floatingPoint.hpp"
#include <algorithm>

CodeGenerator::CodeGenerator() {
	initialize(std::span<const FunctionParameter>(), std::span<const std::string_view>());
}

void CodeGenerator::initialize(std::span<const FunctionParameter> parameters, std::span<const std::string_view> functionNames) {
	registerToLastUsage.clear();
	virtualRegisterToLocation.clear();
	for (i64 i = 0; i < i64(std::size(registerAllocations)); i++) {
		registerAllocations[i] = std::nullopt;
	}
	currentInstructionIndex = 0;
	this->parameters = parameters;
	stackMemoryAllocated = 0;
	stackAllocations.clear();
	a.reset();
}

const auto inputArrayRegister = CodeGenerator::INTEGER_FUNCTION_INPUT_REGISTERS[0];
const auto outputArrayRegister = CodeGenerator::INTEGER_FUNCTION_INPUT_REGISTERS[1];
const auto arraySizeRegister = CodeGenerator::INTEGER_FUNCTION_INPUT_REGISTERS[2];

MachineCode CodeGenerator::compile(
	const std::vector<IrOp>& irCode, 
	std::span<const std::string_view> functionNames, 
	std::span<const FunctionParameter> parameters) {
	initialize(parameters, functionNames);
	computeRegisterLastUsage(irCode);

	const auto someFreeRegister = INTEGER_FUNCTION_INPUT_REGISTERS[2];
	const auto indexRegister = Reg64::RAX;

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
			[&](const ReturnOp& op) {
				add(op.returnedRegister);
			}
		}, op);
	}
}

void CodeGenerator::computeRegisterFirstAssigned(const std::vector<IrOp>& irCode) {
	for (i64 i = 0; i < i64(irCode.size()); i++) {
		callWithDestination(
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

	const auto stackMemoryAllocatedTotal = YMM_REGISTER_SIZE + stackMemoryAllocated;
	/*if (stackMemoryAllocatedTotal > 0)*/ {
		a.push(Reg64::RBP, offset());
		a.mov(Reg64::RBP, Reg64::RSP, offset());
		// Round down (because the stack grows down) to a multiple of 32.
		a.and_(Reg8::BPL, 0b11100000, offset());
		a.sub(Reg64::RBP, u32(stackMemoryAllocatedTotal), offset());
	}

	/*if (stackMemoryAllocated > 0)*/ {
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

	if (registerIsParamter(parameters, reg)) {
		const auto offset = reg * YMM_REGISTER_SIZE;
		location.memoryLocation = RegisterConstantOffsetLocation{
			.registerWithAddress = inputArrayRegister,
			.offset = offset,
		};
		a.vmovaps(*location.registerLocation, inputArrayRegister, u32(offset));
		return actualRegister;
	}

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
	virtualRegisterToSpillLocation.memoryLocation = RegisterConstantOffsetLocation{
		.registerWithAddress = Reg64::RBP,
		.offset = baseOffset.baseOffset
	};
	a.vmovaps(Reg64::RBP, baseOffset.baseOffset, virtualRegisterToSpillRegisterLocation);
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
	for (i64 registerIndex = 0; registerIndex < YMM_REGISTER_COUNT; registerIndex++) {
		if (!registerAllocations[registerIndex].has_value()) {
			continue;
		}

		// All YMM registers are caller saved.
		const bool isCallerSaved = true;
		if (!isCallerSaved) {
			continue;
		}

		const auto reg = RegYmm(registerIndex);
		const auto memory = stackAllocate(YMM_REGISTER_SIZE, YMM_REGISTER_ALIGNMENT);
		a.vmovaps(STACK_TOP_REGISTER, memory.baseOffset, reg);
		registerAllocations[registerIndex] = std::nullopt;

		const auto locationIt = virtualRegisterToLocation.find(registerIndex);
		if (locationIt == virtualRegisterToLocation.end()) {
			ASSERT_NOT_REACHED();
			continue;
		}
		auto& location = locationIt->second;
		if (!location.memoryLocation.has_value()) {
			const auto memory = stackAllocate(YMM_REGISTER_SIZE, YMM_REGISTER_ALIGNMENT);
			location.memoryLocation = memory.location();
			a.vmovaps(STACK_TOP_REGISTER, memory.baseOffset, reg);
		}
		locationIt->second.registerLocation = std::nullopt;
	}

	a.call(functionNameLabel(op.functionName));

	const auto destination = getRegisterLocation(op.destination);
	const auto returnRegister = RegYmm::YMM0;
	a.vmovaps(destination, returnRegister);
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

AddressLabel CodeGenerator::functionNameLabel(std::string_view name)  {
	for (AddressLabel label = 0; label < functionNames.size(); label++) {
		if (functionNames[label] == name) {
			return label;
		}
	}
	ASSERT_NOT_REACHED();
	return 0;
}

CodeGenerator::RegisterConstantOffsetLocation CodeGenerator::BaseOffset::location() const {
	return RegisterConstantOffsetLocation{
		.registerWithAddress = CodeGenerator::STACK_TOP_REGISTER,
		.offset = baseOffset,
	};
}
