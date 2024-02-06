#include "evaluateAst.hpp"
#include "utils/asserts.hpp"
#include "utils/format.hpp"
#include <vector>

struct State {
	std::span<const FunctionParameter> parameters;
	std::span<const float> arguments;
};

static Result<Real, std::string> evaluateExpr(const State& state, const Expr* expr);
static Result<Real, std::string> evaluateBinaryOp(const State& state, Real lhs, Real rhs, BinaryOpType op);
static Result<Real, std::string> evaulateUnaryUp(const State& state, Real operand, UnaryOpType op);
static Result<Real, std::string> getVariable(const State& state, std::string_view identifier);
//static Real resolve;

Result<Real, std::string> evaluateAst(
	const Expr* expr, 
	std::span<const FunctionParameter> parameters, 
	std::span<const float> arguments) {
	ASSERT(parameters.size() == arguments.size());
	State state{
		.parameters = parameters,
		.arguments = arguments,
	};
	return evaluateExpr(state, expr);
}

Result<Real, std::string> evaluateExpr(const State& state, const Expr* expr) {
	switch (expr->type) {
		using enum ExprType;

	case BINARY: {
		const auto binaryExpr = static_cast<const BinaryExpr*>(expr);
		const auto lhs = evaluateExpr(state, binaryExpr->lhs);
		TRY(lhs);
		const auto rhs = evaluateExpr(state, binaryExpr->rhs);
		TRY(rhs);
		return evaluateBinaryOp(state, lhs.ok(), rhs.ok(), binaryExpr->op);
	}

	case UNARY: {
		const auto unaryExpr = static_cast<const UnaryExpr*>(expr);
		const auto operand = evaluateExpr(state, unaryExpr->operand);
		TRY(operand);
		return evaulateUnaryUp(state, operand.ok(), unaryExpr->op);
	}

	case CONSTANT: {
		const auto constantExpr = static_cast<const ConstantExpr*>(expr);
		return constantExpr->value;
	}

	case IDENTIFIER: {
		const auto identifierExpr = static_cast<const IdentifierExpr*>(expr);
		return getVariable(state, identifierExpr->identifier);
	}

	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}

Result<Real, std::string> evaluateBinaryOp(const State& state, Real lhs, Real rhs, BinaryOpType op) {
	switch (op) {
	case BinaryOpType::ADD: return lhs + rhs;
	case BinaryOpType::SUBTRACT: return lhs - rhs;
	case BinaryOpType::MULTIPLY: return lhs * rhs;
	case BinaryOpType::DIVIDE: return lhs / rhs;
	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}

Result<Real, std::string> evaulateUnaryUp(const State& state, Real operand, UnaryOpType op) {
	switch (op) {
		using enum UnaryOpType;
	case NEGATE:
		return -operand;
	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}

Result<Real, std::string> getVariable(const State& state, std::string_view identifier) {
	for (int i = 0; i < state.parameters.size(); i++) {
		if (state.parameters[i].name == identifier) {
			return state.arguments[i];
		}
	}
	// Compiler was complaining the it could convert identifier to std::string or std::string_view. Also it said that it might be because of constness. but this works I guess. Already tried the basic things that might solve it but it didin'didn't work so I am using this.
	const std::string_view& a = identifier;
	return ResultErr(format("variable '%' does not exist", a));
}