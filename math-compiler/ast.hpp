#pragma once

#include "token.hpp"
#include "sourceInfo.hpp"

// Using Expr* pointers instead of references to prevent bugs like writing auto x = someExpr() and then passing it into some struct. Which would leave a dangling reference. To fix this you would need to write auto&.

enum class ExprType {
	CONSTANT,
	BINARY,
	IDENTIFIER,
};

struct Expr {
	Expr(ExprType type, i64 start, i64 end);
	const ExprType type;
	SourceLocation sourceLocation;
};

using Real = float;

struct ConstantExpr : public Expr {
	ConstantExpr(Real value, i64 start, i64 end);

	Real value;
};

enum class BinaryOpType {
	PLUS,
	MINUS,
	MULTIPLY,
	DIVIDE,
};

struct BinaryExpr : public Expr {
	BinaryExpr(Expr* lhs, Expr* rhs, BinaryOpType op, i64 start, i64 end);
	Expr* lhs;
	Expr* rhs;
	BinaryOpType op;
};

struct IdentifierExpr : public Expr {
	IdentifierExpr(std::string_view identifier, i64 start, i64 end);

	std::string_view identifier;
};

struct Ast {
	Expr* root;

	//u8* data;
};
