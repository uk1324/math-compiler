#include "irVm.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"

#define TRY(expression) \
	do { \
		if (expression == Status::ERROR) { \
			return Status::ERROR; \
		} \
	} while (false)

Result<Real, const std::string&> IrVm::execute(const std::vector<IrOp>& instructions, std::span<const float> arguments) {
	initialize(arguments);
	for (const auto& op : instructions) {
		if (const auto returnOp = std::get_if<ReturnOp>(&op)) {
			if (!registerExists(returnOp->returnedRegister)) {
				error("return instruction register doesn't exist");
				return ResultErr<const std::string&>(errorMessage);
				//return ResultErr(errorMessage);
			}
			return registers[returnOp->returnedRegister];
		}
		const auto status = executeOp(op);
	}
	// Code didn't have a return.
	error("code doesn't have a return instruction");
	return ResultErr<const std::string&>(errorMessage);
	//return ResultErr(errorMessage);
}

void IrVm::initialize(std::span<const float> arguments) {
	this->arguments = arguments;
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
		[&](const MultiplyOp& op) { return executeMultiplyOp(op); },
		[&](const ReturnOp& op) {
			ASSERT_NOT_REACHED();
			return Status::ERROR;
		},
	}, op);
}

void IrVm::executeLoadConstantOp(const LoadConstantOp& op) {
	allocateRegisterIfNotExists(op.destination);
	setRegister(op.destination, op.constant);
	registers[op.destination] = op.constant;
}

// Should the destination register exist before compiling this instruction or is it a compiler error?
#define BASIC_BINARY_OP(op_symbol) \
	if (!registerExists(op.lhs) || !registerExists(op.rhs)) { \
		return Status::ERROR; \
	} \
	allocateRegisterIfNotExists(op.destination); \
	setRegister(op.destination, getRegister(op.lhs) op_symbol getRegister(op.rhs)); \
	return Status::OK;

IrVm::Status IrVm::executeAddOp(const AddOp& op) {
	BASIC_BINARY_OP(+)
}

IrVm::Status IrVm::executeMultiplyOp(const MultiplyOp& op) {
	BASIC_BINARY_OP(*)
}

void IrVm::allocateRegisterIfNotExists(Register index) {
	// index = 0 size must be at least 1
	// size > index not >=
	if (static_cast<i64>(registers.size()) <= index) {
		registers.resize(index + 1);
	}
}

bool IrVm::registerExists(Register index) {
	return index < static_cast<i64>(registers.size());
}

void IrVm::setRegister(Register index, Real value) {
	if (index < arguments.size()) {
		// Arguments can't be changed.
		ASSERT_NOT_REACHED();
		return;
	}
}

const Real& IrVm::getRegister(Register index) const {
	if (index < arguments.size()) {
		return arguments[index];
	}
	return registers[index];
}
