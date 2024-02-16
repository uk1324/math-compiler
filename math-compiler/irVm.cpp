#include "irVm.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"
#include "ffiUtils.hpp"
#include <immintrin.h>

#define TRY(expression) \
	do { \
		if (expression == Status::ERROR) { \
			return Status::ERROR; \
		} \
	} while (false)

Result<Real, std::string> IrVm::execute(
	const std::vector<IrOp>& instructions, 
	std::span<const float> arguments,
	std::span<const FunctionInfo> functionInfo) {
	initialize(arguments, functionInfo);
	for (const auto& op : instructions) {
		if (const auto returnOp = std::get_if<ReturnOp>(&op)) {
			if (!registerExists(returnOp->returnedRegister)) {
				registerDoesNotExistError(returnOp->returnedRegister);
				return ResultErr(std::string(errorMessage));
			}
			return getRegister(returnOp->returnedRegister);
		}
		const auto status = executeOp(op);
		if (status == Status::ERROR) {
			return ResultErr(std::string(errorMessage));
		}
	}
	// Code didn't have a return.
	error("code doesn't have a return instruction");
	return ResultErr(std::string(errorMessage));
	//return ResultErr(errorMessage);
}

void IrVm::initialize(std::span<const float> arguments, std::span<const FunctionInfo> functionInfo) {
	this->arguments = arguments;
	this->functionInfo = functionInfo;
	errorMessage = std::string_view();
	registers.clear();
}

IrVm::Status IrVm::executeOp(const IrOp& op) {
	return std::visit(overloaded{
		[&](const LoadConstantOp& op) { 
			executeLoadConstantOp(op); 
			return Status::OK;
		},
		[&](const AddOp& op) { return executeAddOp(op); },
		[&](const SubtractOp& op) { return executeOp(op); },
		[&](const MultiplyOp& op) { return executeMultiplyOp(op); },
		[&](const DivideOp& op) { return executeOp(op); },
		[&](const XorOp& op) { return executeOp(op); },
		[&](const NegateOp& op) { return executeOp(op); },
		[&](const FunctionOp& op) { return executeOp(op); },
		[&](const ReturnOp& op) {
			ASSERT_NOT_REACHED();
			return Status::ERROR;
		},
	}, op);
}

void IrVm::executeLoadConstantOp(const LoadConstantOp& op) {
	allocateRegisterIfNotExists(op.destination);
	setRegister(op.destination, op.constant);
}

// Should the destination register exist before compiling this instruction or is it a compiler error?
#define BASIC_BINARY_OP(op_symbol) \
	if (!registerExists(op.lhs)) { \
		return registerDoesNotExistError(op.lhs); \
	} \
	if (!registerExists(op.rhs)) { \
		return registerDoesNotExistError(op.rhs); \
	} \
	allocateRegisterIfNotExists(op.destination); \
	setRegister(op.destination, getRegister(op.lhs) op_symbol getRegister(op.rhs)); \
	return Status::OK;

IrVm::Status IrVm::executeAddOp(const AddOp& op) {
	BASIC_BINARY_OP(+)
}

IrVm::Status IrVm::executeOp(const SubtractOp& op) {
	BASIC_BINARY_OP(-);
}

IrVm::Status IrVm::executeMultiplyOp(const MultiplyOp& op) {
	BASIC_BINARY_OP(*)
}

IrVm::Status IrVm::executeOp(const DivideOp& op) {
	BASIC_BINARY_OP(/);
}

IrVm::Status IrVm::executeOp(const XorOp& op) {
	if (!registerExists(op.lhs)) {
		return registerDoesNotExistError(op.lhs);
	}
	if (!registerExists(op.rhs)) {
		return registerDoesNotExistError(op.rhs);
	}
	allocateRegisterIfNotExists(op.destination);
	setRegister(op.destination, std::bit_cast<float>(std::bit_cast<u32>(getRegister(op.lhs)) xor std::bit_cast<u32>(getRegister(op.rhs))));
	return Status::OK;
}

IrVm::Status IrVm::executeOp(const NegateOp& op) {
	allocateRegisterIfNotExists(op.destination);
	if (!registerExists(op.operand)) {
		return registerDoesNotExistError(op.operand);
	}
	setRegister(op.destination, -getRegister(op.operand));
	return Status::OK;
}

IrVm::Status IrVm::executeOp(const FunctionOp& op) {
	const auto function = std::find_if(
		functionInfo.begin(), functionInfo.end(), 
		[&](const FunctionInfo& f) { return f.name == op.functionName; });

	if (function == functionInfo.end()) {
		return error("function '%' doesn't exist", &op.functionName);
	}

	if (function->arity != op.arguments.size()) {
		return error("'%' expected % arguments found %", &op.functionName, function->arity, op.arguments.size());
	}

	std::vector<Real> arguments;
	for (const auto& argument : op.arguments) {
		if (!registerExists(argument)) {
			return registerDoesNotExistError(argument);
		}
		arguments.push_back(getRegister(argument));
	}

	allocateRegisterIfNotExists(op.destination);
	setRegister(op.destination, callSimdVectorCall(function->address, arguments));
	
	return Status::OK;
}

void IrVm::allocateRegisterIfNotExists(Register index) {
	// index = 0 size must be at least 1
	// size > index not >=
	if (static_cast<i64>(registers.size()) <= index) {
		registers.resize(index + 1);
	}
}

bool IrVm::registerExists(Register index) {
	return (index < i64(registers.size())) ||
		(index < i64(arguments.size()));
}

void IrVm::setRegister(Register index, Real value) {
	if (index < i64(arguments.size())) {
		// Arguments can't be changed.
		ASSERT_NOT_REACHED();
		return;
	}
	registers[index] = value;
}

IrVm::Status IrVm::registerDoesNotExistError(Register reg) {
	return error("r% doesn't exist", reg);
}

const Real& IrVm::getRegister(Register index) const {
	if (index < i64(arguments.size())) {
		return arguments[index];
	}
	return registers[index];
}
