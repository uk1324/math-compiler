#include "ast.hpp"

ConstantExpr::ConstantExpr(Real value)
	: Expr{ ExprType::CONSTANT }
	, value(value) {}

BinaryExpr::BinaryExpr(Expr* lhs, Expr* rhs, BinaryOpType op)
	: Expr{ ExprType::BINARY }
	, lhs(lhs)
	, rhs(rhs)
	, op(op) {}

//VariableExpr::VariableExpr(std::string_view identifier) 
//	: Expr{ ExprType::VARIABLE } {
//}

IdentifierExpr::IdentifierExpr(std::string_view identifier)
	: Expr{ ExprType::IDENTIFIER }
	, identifier(identifier) {}
