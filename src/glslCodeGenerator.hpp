#pragma once

#include "ir.hpp"
#include "input.hpp"
#include <span>
#include <ostream>

struct GlslCodeGenerator {
	void initialize(std::ostream& output, std::span<const Variable> variables, std::span<const FunctionInfo> functions);

	void compile(
		std::ostream& output,
		const std::vector<IrOp>& irCode,
		std::span<const FunctionInfo> functions,
		std::span<const Variable> variables);

	void generate(const LoadConstantOp& op);
	void generate(const LoadVariableOp& op);
	void generate(const AddOp& op);
	void generate(const SubtractOp& op);
	void generate(const MultiplyOp& op);
	void generate(const DivideOp& op);
	void generate(const ExponentiateOp& op);
	void generate(const XorOp& op);
	void generate(const NegateOp& op);
	void generate(const FunctionOp& op);
	void generate(const ReturnOp& op);
	
	void outRegisterName(Register reg);
	void outRegisterEquals(Register reg);
	void outVariableName(i64 variableIndex);
	void outInfixBinaryOp(Register destination, Register lhs, Register rhs, const char* op);

	std::span<const Variable> variables;
	std::span<const FunctionInfo> functions;
	std::ostream* out_ = nullptr;
	std::ostream& out();
};