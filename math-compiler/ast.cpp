#include "ast.hpp"

ConstantExpr::ConstantExpr(Real value, i64 start, i64 end)
	: Expr(ExprType::CONSTANT, start, end)
	, value(value) {}

BinaryExpr::BinaryExpr(Expr* lhs, Expr* rhs, BinaryOpType op, i64 start, i64 end)
	: Expr(ExprType::BINARY, start, end)
	, lhs(lhs)
	, rhs(rhs)
	, op(op) {}

IdentifierExpr::IdentifierExpr(std::string_view identifier, i64 start, i64 end)
	: Expr(ExprType::IDENTIFIER, start, end)
	, identifier(identifier) {}

Expr::Expr(ExprType type, i64 start, i64 end) 
	: type(type)
	, sourceLocation(SourceLocation::fromStartEnd(start, end)) {}
