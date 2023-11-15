#pragma once

#include "ir.hpp"
#include <optional>

struct IrVm {
	enum class Status {
		OK, ERROR
	};

	std::optional<Real> execute(const std::vector<IrOp>& instructions);
	Status executeOp(const IrOp& op);
	void executeLoadConstantOp(const LoadConstantOp& op);
	Status executeAddOp(const AddOp& op);
	Status executeMultiplyOp(const MultiplyOp& op);

	void allocateRegisterIfNotExists(Register index);
	bool registerExists(Register index);

	std::vector<Real> registers;
};