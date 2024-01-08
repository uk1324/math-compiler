#include "evaluateAst.hpp"
#include "utils/asserts.hpp"
#include <vector>

struct State {
	std::span<const FunctionParameter> parameters;
	std::span<const float> arguments;
};

static Result<Real, std::string> evaluateExpr(const State& state, const Expr* expr);
static Result<Real, std::string> evaulateBinaryOp(const State& state, Real lhs, Real rhs, BinaryOpType op);

//static Real resolve;

Result<Real, std::string> evaluateAst(
	const Expr* expr, 
	std::span<const FunctionParameter> parameters, 
	std::span<const float> arguments) {
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
		return evaulateBinaryOp(state, lhs.ok(), rhs.ok(), binaryExpr->op);
	}

	case CONSTANT: {
		const auto constantExpr = static_cast<const ConstantExpr*>(expr);
		return constantExpr->value;
	}

	case IDENTIFIER: {
		const auto identifierExpr = static_cast<const IdentifierExpr*>(expr);
		identifierExpr->identifier;

	}

	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}

Result<Real, std::string> evaulateBinaryOp(const State& state, Real lhs, Real rhs, BinaryOpType op) {
	switch (op) {
	case BinaryOpType::PLUS: return lhs + rhs;
	case BinaryOpType::MINUS: return lhs - rhs;
	case BinaryOpType::MULTIPLY: return lhs * rhs;
	case BinaryOpType::DIVIDE: return lhs / rhs;

	default:
		ASSERT_NOT_REACHED();
		return 0.0f;
	}
}