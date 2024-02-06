#pragma once

#include "ir.hpp"
#include "utils/format.hpp"
#include "utils/result.hpp"
#include <optional>
#include <span>

struct IrVm {
	enum class Status {
		OK, ERROR
	};

	Result<Real, std::string> execute(const std::vector<IrOp>& instructions, std::span<const float> arguments);
	void initialize(std::span<const float> arguments);
	Status executeOp(const IrOp& op);
	void executeLoadConstantOp(const LoadConstantOp& op);
	Status executeAddOp(const AddOp& op);
	Status executeOp(const SubtractOp& op);
	Status executeMultiplyOp(const MultiplyOp& op);
	Status executeOp(const DivideOp& op);
	Status executeOp(const XorOp& op);
	Status executeOp(const NegateOp& op);

	void allocateRegisterIfNotExists(Register index);
	bool registerExists(Register index);

	const Real& getRegister(Register index) const;
	void setRegister(Register index, Real value);

	template <typename ...Args>
	Status error(const char* format, const Args&... args);
	Status registerDoesNotExistError(Register reg);

	std::string errorMessage;
	std::vector<Real> registers;
	std::span<const float> arguments;
};

template<typename ...Args>
IrVm::Status IrVm::error(const char* format, const Args & ...args) {
	// @Performance could use a custom string view and use put() to print the message into the string buffer.
	errorMessage = ::format(format, args...);
	return Status::ERROR;
}
