#include "irCompiler.hpp"
#include "utils/asserts.hpp"
#include <iostream>

//#define IR_COMPILER_DEBUG_PRINT_ADDED_INSTRUCTIONS

IrCompiler::IrCompiler() {
	initialize(std::span<const Variable>(), std::span<const FunctionInfo>(), nullptr);
}

void IrCompiler::initialize(
	std::span<const Variable> parameters, 
	std::span<const FunctionInfo> functionInfo,
	IrCompilerMessageReporter* reporter) {
	generatedIrCode.clear();
	this->parameters = parameters;
	this->reporter = reporter;
	allocatedRegistersCount = parameters.size();
}

std::optional<const std::vector<IrOp>&> IrCompiler::compile(
	const Ast& ast,
	std::span<const Variable> parameters,
	std::span<const FunctionInfo> functionInfo,
	IrCompilerMessageReporter& reporter) {
	initialize(parameters, functionInfo, &reporter);

	try {
		const auto result = compileExpression(ast.root);
		addOp(ReturnOp{
			.returnedRegister = result.result
		});
		return generatedIrCode;
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
		CASE_EXPR(UNARY, UnaryExpr);
		CASE_EXPR(IDENTIFIER, IdentifierExpr);
		CASE_EXPR(FUNCTION, FunctionExpr);
	
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
	case BinaryOpType::ADD:
		addOp(AddOp{ .destination = destination, .lhs = lhs.result, .rhs = rhs.result });
		break;

	case BinaryOpType::SUBTRACT:
		addOp(SubtractOp{ .destination = destination, .lhs = lhs.result, .rhs = rhs.result });
		break;

	case BinaryOpType::MULTIPLY:
		addOp(MultiplyOp{ .destination = destination, .lhs = lhs.result, .rhs = rhs.result });
		break;

	case BinaryOpType::DIVIDE:
		addOp(DivideOp{ .destination = destination, .lhs = lhs.result, .rhs = rhs.result });
		break;
	}
	
	return ExprResult{ .result = destination };
}

IrCompiler::ExprResult IrCompiler::compileUnaryExpr(const UnaryExpr& expr) {
	const auto operand = compileExpression(expr.operand);
	const auto destination = allocateRegister();

	switch (expr.op) {
		using enum UnaryOpType;
		case NEGATE:
			addOp(NegateOp{ .destination = destination, .operand = operand.result });
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
			const auto destination = allocateRegister();
			addOp(LoadVariableOp{ .destination = destination, .variableIndex = i });
			return ExprResult{ .result = destination };
		}
	}
	throwError(UndefinedVariableIrCompilerError{
		.undefinedVariableName = expr.identifier,
		.location = expr.sourceLocation
	});
}

IrCompiler::ExprResult IrCompiler::compileFunctionExpr(const FunctionExpr& expr) {
	const auto destination = allocateRegister();
	FunctionOp op{ .destination = destination, .functionName = expr.functionName };
	for (const auto& argumentExpr : expr.arguments) {
		const auto arg = compileExpression(argumentExpr).result;
		op.arguments.push_back(arg);
	}
	addOp(op);
	return ExprResult{ .result = destination };
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
