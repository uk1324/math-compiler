#pragma once

#include "ir.hpp"
#include <optional>
#include <span>
#include "input.hpp"
#include "irCompilerMessageReporter.hpp"

struct IrCompiler {
	struct ExprResult {
		Register result;
	};

	struct FunctionInfo {
		std::string_view name;
		i64 arity;
	};

	struct CompilerError {};

	IrCompiler();
	void initialize(
		std::span<const FunctionParameter> parameters, 
		std::span<const FunctionInfo> functionInfo,
		IrCompilerMessageReporter* reporter);

	std::optional<const std::vector<IrOp>*> compile(
		const Ast& ast,
		std::span<const FunctionParameter> parameters,
		std::span<const FunctionInfo> functionInfo,
		IrCompilerMessageReporter& reporter);

	ExprResult compileExpression(const Expr* expr);
	ExprResult compileConstantExpr(const ConstantExpr& expr);
	ExprResult compileBinaryExpr(const BinaryExpr& expr);
	ExprResult compileUnaryExpr(const UnaryExpr& expr);
	void createRegistersForVariables();
	ExprResult compileIdentifierExpr(const IdentifierExpr& expr);
	ExprResult compileFunctionExpr(const FunctionExpr& expr);

	i64 allocatedRegistersCount = 0;
	Register allocateRegister();

	void addOp(const IrOp& op);

	[[noreturn]] void throwError(const IrCompilerError& error);

	std::vector<IrOp> generatedIrCode;

	std::span<const FunctionParameter> parameters;
	IrCompilerMessageReporter* reporter;
};