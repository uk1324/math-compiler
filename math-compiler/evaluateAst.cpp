#include "evaluateAst.hpp"
#include "utils/asserts.hpp"

Real evaluateExpr(const Expr* expr) {
	switch (expr->type)
	{
	case ExprType::BINARY: {
		const auto binaryExpr = static_cast<const BinaryExpr*>(expr);
		const auto lhs = evaluateExpr(binaryExpr->lhs);
		const auto rhs = evaluateExpr(binaryExpr->rhs);
		return evaulateBinaryOp(lhs, rhs, binaryExpr->op);
	}
		
	case ExprType::CONSTANT: {
		const auto constantExpr = static_cast<const ConstantExpr*>(expr);
		return constantExpr->value;
	}

	default:
		ASSERT_NOT_REACHED();
		return 0.0f;
	}
}

Real evaulateBinaryOp(Real lhs, Real rhs, BinaryOpType op) {
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