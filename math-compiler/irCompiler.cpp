#include "irCompiler.hpp"
#include "utils/asserts.hpp"
#include <iostream>

//#define IR_COMPILER_DEBUG_PRINT_ADDED_INSTRUCTIONS

IrCompiler::IrCompiler() {
	initialize(std::span<const FunctionParameter>(), nullptr);
}

void IrCompiler::initialize(std::span<const FunctionParameter> parameters, IrCompilerMessageReporter* reporter) {
	generatedIrCode.clear();
	this->parameters = parameters;
	this->reporter = reporter;
}

std::optional<const std::vector<IrOp>*> IrCompiler::compile(
	const Ast& ast, 
	const std::span<const FunctionParameter> parameters,
	IrCompilerMessageReporter& reporter) {
	initialize(parameters, &reporter);

	try {
		const auto result = compileExpression(ast.root);
		addOp(ReturnOp{
			.returnedRegister = result.result
		});
		return &generatedIrCode;
	} catch (const CompilerError&) {
		return std::nullopt;
	}


}

IrCompiler::ExprResult IrCompiler::compileExpression(const Expr* expr) {
#define CASE_EXPR(ENUM_NAME, TypeName) \
	case ExprType::ENUM_NAME: return compile##TypeName(*static_cast<const TypeName*>(expr))

	switch (expr->type) {
		CASE_EXPR(CONSTANT, ConstantExpr);
		CASE_EXPR(BINARY, BinaryExpr);
		CASE_EXPR(IDENTIFIER, IdentifierExpr);
	
	default:
		ASSERT_NOT_REACHED();
		return ExprResult{ .result = 0xCCCCCCCC };
	}
}

IrCompiler::ExprResult IrCompiler::compileConstantExpr(const ConstantExpr& expr) {
	const auto destination = allocateRegister();
	addOp(LoadConstantOp{ 
		.destination = destination,
		.constant = expr.value
	});
	return ExprResult{ .result = destination };
}

IrCompiler::ExprResult IrCompiler::compileBinaryExpr(const BinaryExpr& expr) {
	const auto lhs = compileExpression(expr.lhs);
	const auto rhs = compileExpression(expr.rhs);
	const auto destination = allocateRegister();
	switch (expr.op) {
	case BinaryOpType::PLUS:
		addOp(AddOp{
			.destination = destination,
			.lhs = lhs.result,
			.rhs = rhs.result
		});
		break;

	case BinaryOpType::MULTIPLY:
		addOp(MultiplyOp{
			.destination = destination,
			.lhs = lhs.result,
			.rhs = rhs.result
		});
		break;

	default:
		ASSERT_NOT_REACHED();
		break;
	}
	
	return ExprResult{ .result = destination };
}

void IrCompiler::createRegistersForVariables() {
	for (i32 i = 0; i < parameters.size(); i++) {
		const auto reg = allocateRegister();
	}
}

IrCompiler::ExprResult IrCompiler::compileIdentifierExpr(const IdentifierExpr& expr) {
	for (i32 i = 0; i < parameters.size(); i++) {
		if (parameters[i].name == expr.identifier) {
			return ExprResult{ .result = i };
		}
	}
	throwError(UndefinedVariableIrCompilerError{
		.undefinedVariableName = expr.identifier,
		.location = expr.sourceLocation
	});
}

Register IrCompiler::allocateRegister() {
	const Register allocated = allocatedRegistersCount;
	allocatedRegistersCount++;
	return allocated;
}

void IrCompiler::addOp(const IrOp& op) {
	generatedIrCode.push_back(op);
	#ifdef IR_COMPILER_DEBUG_PRINT_ADDED_INSTRUCTIONS
	printIrOp(std::cout, op);
	#endif
}

[[noreturn]] void IrCompiler::throwError(const IrCompilerError& error) {
	reporter->onError(error);
	throw CompilerError{};
}
