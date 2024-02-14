#pragma once

#include "token.hpp"
#include "sourceInfo.hpp"
#include <span>

// Using Expr* pointers instead of references to prevent bugs like writing auto x = someExpr() and then passing it into some struct. Which would leave a dangling reference. To fix this you would need to write auto&.

enum class ExprType {
	CONSTANT,
	BINARY,
	UNARY,
	IDENTIFIER,
	FUNCTION,
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
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE,
};

struct BinaryExpr : public Expr {
	BinaryExpr(Expr* lhs, Expr* rhs, BinaryOpType op, i64 start, i64 end);
	Expr* lhs;
	Expr* rhs;
	BinaryOpType op;
};

enum class UnaryOpType {
	NEGATE,
};

struct UnaryExpr : public Expr {
	UnaryExpr(Expr* operand, UnaryOpType op, i64 start, i64 end);

	Expr* operand;
	UnaryOpType op;
};

struct IdentifierExpr : public Expr {
	IdentifierExpr(std::string_view identifier, i64 start, i64 end);

	std::string_view identifier;
};

struct FunctionExpr : public Expr {
	FunctionExpr(std::string_view functionName, std::span<const Expr*> arguments, i64 start, i64 end);

	std::string_view functionName;
	std::span<const Expr*> arguments;
};

struct Ast {
	Expr* root;

	//u8* data;
};
