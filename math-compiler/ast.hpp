#pragma once

#include "token.hpp"

// Using Expr* pointers instead of references to prevent bugs like writing auto x = someExpr() and then passing it into some struct. Which would leave a dangling reference. To fix this you would need to write auto&.

enum class ExprType {
	CONSTANT,
	BINARY,
	IDENTIFIER,
};

struct Expr {
	const ExprType type;
};

using Real = float;

struct ConstantExpr : public Expr {
	ConstantExpr(Real value);

	Real value;
};

enum class BinaryOpType {
	PLUS,
	MINUS,
	MULTIPLY,
	DIVIDE,
};

struct BinaryExpr : public Expr {
	BinaryExpr(Expr* lhs, Expr* rhs, BinaryOpType op);
	Expr* lhs;
	Expr* rhs;
	BinaryOpType op;
};

struct IdentifierExpr : public Expr {
	IdentifierExpr(std::string_view identifier);

	std::string_view identifier;
};

struct Ast {
	Expr* root;

	//u8* data;
};
