#include "irVm.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"

#define TRY(expression) \
	do { \
		if (expression == Status::ERROR) { \
			return Status::ERROR; \
		} \
	} while (false)

std::optional<Real> IrVm::execute(const std::vector<IrOp>& instructions) {
	for (const auto& op : instructions) {
		if (const auto returnOp = std::get_if<ReturnOp>(&op)) {
			if (!registerExists(returnOp->returnedRegister)) {
				return std::nullopt;
			}
			return registers[returnOp->returnedRegister];
		}
		const auto status = executeOp(op);
	}
	// Code didn't have a return.
	return std::nullopt;
}

IrVm::Status IrVm::executeOp(const IrOp& op) {
	return std::visit(overloaded{
		[&](const LoadConstantOp& op) { 
			executeLoadConstantOp(op); 
			return Status::OK;
		},
		[&](const AddOp& op) { 
			return executeAddOp(op); 
		},
		[&](const MultiplyOp& op) {
			return executeMultiplyOp(op);
		},
		[&](const ReturnOp& op) {
			ASSERT_NOT_REACHED();
			return Status::ERROR;
		},
	}, op);
}

void IrVm::executeLoadConstantOp(const LoadConstantOp& op) {
	allocateRegisterIfNotExists(op.destination);
	registers[op.destination] = op.constant;
}

#define BASIC_BINARY_OP(op_symbol) \
	if (!registerExists(op.lhs) || !registerExists(op.rhs)) { \
		return Status::ERROR; \
	} \
	allocateRegisterIfNotExists(op.destination); \
	registers[op.destination] = registers[op.lhs] op_symbol registers[op.rhs]; \
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
