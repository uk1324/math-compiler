#pragma once

#include <variant>
#include <vector>
#include <ostream>
#include "ast.hpp"
#include "utils/ints.hpp"

using Register = i64;

struct LoadConstantOp {
	Register destination;
	Real constant;
};

struct AddOp {
	Register destination;
	Register lhs;
	Register rhs;
};

struct SubtractOp {
	Register destination;
	Register lhs;
	Register rhs;
};

struct MultiplyOp {
	Register destination;
	Register lhs;
	Register rhs;
};

struct DivideOp {
	Register destination;
	Register lhs;
	Register rhs;
};

struct XorOp {
	Register destination;
	Register lhs;
	Register rhs;
};

struct NegateOp {
	Register destination;
	Register operand;
};

struct ReturnOp {
	Register returnedRegister;
};

using IrOp = std::variant<
	LoadConstantOp,
	AddOp,
	SubtractOp,
	MultiplyOp,
	DivideOp,
	XorOp,
	NegateOp,
	ReturnOp
>;

void printIrOp(std::ostream& out, const IrOp& op);
void printIrCode(std::ostream& out, const std::vector<IrOp>& code);