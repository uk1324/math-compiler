#pragma once

#include <variant>
#include <vector>
#include <ostream>
#include <span>
#include "ast.hpp"
#include "utils/ints.hpp"
#include "input.hpp"

using Register = i64;
bool registerIsParamter(std::span<const FunctionParameter> parameters, Register r);

struct LoadConstantOp {
	Register destination;
	Real constant;

	template<typename Function> 
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

struct AddOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

struct SubtractOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

struct MultiplyOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

struct DivideOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

struct XorOp {
	Register destination;
	Register lhs;
	Register rhs;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

struct NegateOp {
	Register destination;
	Register operand;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	// TODO: Name this something better call with usedRegisterButNotDestination or find some name for it like operands idk.
	void callWithUsedRegisters(Function f) const;
};

// It is assumed that the function has no side effects.
// FunctionOp is followed by zero or more FunctionArgumentsOps. If a FunctionArgumentsOp is not placed right after the FunctionOp it is ignored.
// TODO: What are the issues with using FunctionArgumentOp istead of just storing a vector of Register?
// This allows for invalid representations code generation like last usage instruction cannot be implemented simply by looking at the instruction but the context is needed. Requires adding extra logic in places like code generation for example. 
// If I don't want to allocate then one way could be to create things like UnaryFunctionOp, BinaryFunctionOp and so on, but this would increase the size of every instruction so the simplest thing would probably be to use a custom allocator.
struct FunctionOp {
	Register destination;
	std::string_view functionName;
	std::vector<Register> arguments;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

//struct FunctionArgumentOp {
//	Register argumentRegister;
//};

struct ReturnOp {
	Register returnedRegister;

	template<typename Function>
	void callWithDestination(Function f) const;
	template<typename Function>
	void callWithUsedRegisters(Function f) const;
};

using IrOp = std::variant<
	LoadConstantOp,
	AddOp,
	SubtractOp,
	MultiplyOp,
	DivideOp,
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
void callWithDestination(const IrOp& op, Function f);
template<typename Function>
void callWithUsedRegisters(const IrOp& op, Function f);

template<typename Function>
void LoadConstantOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void LoadConstantOp::callWithUsedRegisters(Function f) const {}

template<typename Function>
void ReturnOp::callWithDestination(Function f) const {}

template<typename Function>
void ReturnOp::callWithUsedRegisters(Function f) const {
	f(returnedRegister);
}

template<typename Function>
void XorOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void XorOp::callWithUsedRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void AddOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void AddOp::callWithUsedRegisters(Function f) const{
	f(lhs);
	f(rhs);
}

template<typename Function>
void SubtractOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void SubtractOp::callWithUsedRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void NegateOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void NegateOp::callWithUsedRegisters(Function f) const {
	f(operand);
}

template<typename Function>
void DivideOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void DivideOp::callWithUsedRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void MultiplyOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void MultiplyOp::callWithUsedRegisters(Function f) const {
	f(lhs);
	f(rhs);
}

template<typename Function>
void FunctionOp::callWithDestination(Function f) const {
	f(destination);
}

template<typename Function>
void FunctionOp::callWithUsedRegisters(Function f) const {
	for (const auto& argument : arguments) {
		f(argument);
	}
}

template<typename Function>
void callWithDestination(const IrOp& op, Function f) {
	std::visit([&f](const auto& v) { v.callWithDestination(f); }, op);
}

template<typename Function>
void callWithUsedRegisters(const IrOp& op, Function f) {
	std::visit([&f](const auto& v) { v.callWithUsedRegisters(f); }, op);
}

