#pragma once

#include "ir.hpp"
#include <optional>
#include <span>
#include "input.hpp"
#include "irCompilerMessageReporter.hpp"

struct IrCompiler {
	/*struct DataType {

	};*/

	struct ExprResult {
		Register result;
	};

	struct CompilerError {};

	IrCompiler();
	void initialize(std::span<const FunctionParameter> parameters, IrCompilerMessageReporter* reporter);

	std::optional<const std::vector<IrOp>*> compile(
		const Ast& ast, 
		std::span<const FunctionParameter> parameters,
		IrCompilerMessageReporter& reporter);

	ExprResult compileExpression(const Expr* expr);
	ExprResult compileConstantExpr(const ConstantExpr& expr);
	ExprResult compileBinaryExpr(const BinaryExpr& expr);
	void createRegistersForVariables();
	ExprResult compileIdentifierExpr(const IdentifierExpr& expr);

	i64 allocatedRegistersCount = 0;
	Register allocateRegister();

	void addOp(const IrOp& op);

	[[noreturn]] void throwError(const IrCompilerError& error);

	std::vector<IrOp> generatedIrCode;

	std::span<const FunctionParameter> parameters;
	IrCompilerMessageReporter* reporter;
};