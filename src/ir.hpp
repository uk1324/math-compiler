#pragma once

#include <variant>
#include <vector>
#include <ostream>
#include <span>
#include "ast.hpp"
#include "utils/ints.hpp"
#include "input.hpp"

using Register = i64;

struct LoadConstantOp {
	Register destination;
	Real constant;

	template<typename Function> 
	void callWithInputRegisters(Function f) const;
	template<typename Function>
	void callWithOutputRegisters(Function f) const;
};

struct LoadVariableOp {
	Register destination;
	i64 variableIndex;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct AddOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct SubtractOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct MultiplyOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct DivideOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct ExponentiateOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct XorOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

struct NegateOp {
	Register destination;
	Register operand;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

// It is assumed that the function has no side effects.
// FunctionOp is followed by zero or more FunctionArgumentsOps. If a FunctionArgumentsOp is not placed right after the FunctionOp it is ignored.
// TODO: What are the issues with using FunctionArgumentOp istead of just storing a vector of Register?
// This allows for invalid representations code generation like last usage instruction cannot be implemented simply by looking at the instruction but the context is needed. Requires adding extra logic in places like code generation for example. 
// If I don't want to allocate then one way could be to create things like UnaryFunctionOp, BinaryFunctionOp and so on, but this would increase the size of every instruction so the simplest thing would probably be to use a custom allocator.
struct FunctionOp {
	Register destination;
	// Maybe instead of storing the name just store a reference to the FunctionInfo.
	std::string_view functionName;
	std::vector<Register> arguments;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

//struct FunctionArgumentOp {
//	Register argumentRegister;
//};

struct ReturnOp {
	Register returnedRegister;

	template<typename Function>
	void callWithOutputRegisters(Function f) const;
	template<typename Function>
	void callWithInputRegisters(Function f) const;
};

using IrOp = std::variant<
	LoadConstantOp,
	LoadVariableOp,
	AddOp,
	SubtractOp,
	MultiplyOp,
	DivideOp,
	ExponentiateOp,
	XorOp,
	NegateOp,
	FunctionOp,
	ReturnOp
>;

void printIrOp(std::ostream& out, const IrOp& op);
void printIrCode(std::ostream& out, const std::vector<IrOp>& code);

// Without using templates this could be implemented with higher order macros.
// Don't know if there is a nice way to have return values from Function f, because you can either have no destination or multiple. The simplest option might be to just return by writing to variables captured variables.
template<typename Function>
void callWithOutputRegisters(const IrOp& op, Function f);
template<typename Function>
void callWithInputRegisters(const IrOp& op, Function f);

template<typename Function>
void LoadConstantOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void LoadConstantOp::callWithInputRegisters(Function f) const {}

template<typename Function>
void ReturnOp::callWithOutputRegisters(Function f) const {}

template<typename Function>
void ReturnOp::callWithInputRegisters(Function f) const {
	f(returnedRegister);
}

template<typename Function>
void XorOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void XorOp::callWithInputRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void AddOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void AddOp::callWithInputRegisters(Function f) const{
	f(lhs);
	f(rhs);
}

template<typename Function>
void SubtractOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void SubtractOp::callWithInputRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void NegateOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void NegateOp::callWithInputRegisters(Function f) const {
	f(operand);
}

template<typename Function>
void DivideOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void DivideOp::callWithInputRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void ExponentiateOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void ExponentiateOp::callWithInputRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void MultiplyOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void MultiplyOp::callWithInputRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void FunctionOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void FunctionOp::callWithInputRegisters(Function f) const {
	for (const auto& argument : arguments) {
		f(argument);
	}
}

template<typename Function>
void callWithOutputRegisters(const IrOp& op, Function f) {
	std::visit([&f](const auto& v) { v.callWithOutputRegisters(f); }, op);
}

template<typename Function>
void callWithInputRegisters(const IrOp& op, Function f) {
	std::visit([&f](const auto& v) { v.callWithInputRegisters(f); }, op);
}

template<typename Function>
void LoadVariableOp::callWithOutputRegisters(Function f) const {
	f(destination);
}

template<typename Function>
void LoadVariableOp::callWithInputRegisters(Function f) const {}
