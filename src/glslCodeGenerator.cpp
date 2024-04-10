#include "glslCodeGenerator.hpp"
#include "utils/overloaded.hpp"
#include "utils/asserts.hpp"
#include <unordered_set>

void GlslCodeGenerator::initialize(std::ostream& output, std::span<const Variable> variables, std::span<const FunctionInfo> functions) {
	this->variables = variables;
	this->functions = functions;
	this->out_ = &output;
}

void GlslCodeGenerator::compile(
	std::ostream& output,
	const std::vector<IrOp>& irCode,
	std::span<const FunctionInfo> functions,
	std::span<const Variable> variables) {

	initialize(output, variables, functions);

	std::unordered_set<Register> usedRegisters;
	for (const auto& op : irCode) {
		callWithOutputRegisters(op, [&usedRegisters](Register r) {
			usedRegisters.insert(r);
		});
	}
	out() << "float result;\n";
	out() << '{';

	for (const auto& r : usedRegisters) {
		// Using variables instead of arrays might be better, because:
		// The register might not be consecutive after optimization passes (this could be fixed by doing another pass).
		// Don't know if the compiler can reuse memory inside an array that is no longer used, but it should be able to do this with variables. This could also be fixed by doing register allocation myself.
		out() << "float ";
		outRegisterName(r);
		out() << ";\n";

	}

	for (const auto& op : irCode) {
		std::visit(overloaded{
			[&](const auto& op) { generate(op); },
		}, op);
	}

	out() << '}';
}

void GlslCodeGenerator::generate(const LoadConstantOp& op) {
	outRegisterEquals(op.destination);
	out() << op.constant << ";\n";
}

void GlslCodeGenerator::generate(const LoadVariableOp& op) {
	// Could do register allocation to not do copies, but the GLSL compiler should opitmize it.
	outRegisterEquals(op.destination);
	outVariableName(op.variableIndex);
	out() << ";\n";
}

void GlslCodeGenerator::generate(const AddOp& op) {
	outInfixBinaryOp(op.destination, op.lhs, op.rhs, "+");
}

void GlslCodeGenerator::generate(const SubtractOp& op) {
	outInfixBinaryOp(op.destination, op.lhs, op.rhs, "-");
}

void GlslCodeGenerator::generate(const MultiplyOp& op) {
	outInfixBinaryOp(op.destination, op.lhs, op.rhs, "*");
}

void GlslCodeGenerator::generate(const DivideOp& op) {
	outInfixBinaryOp(op.destination, op.lhs, op.rhs, "/");
}

void GlslCodeGenerator::generate(const ExponentiateOp& op) {
	outRegisterEquals(op.destination);
	out() << "pow(";
	out() << op.lhs;
	out() << ", ";
	out() << op.rhs;
	out() << ");\n";
}

void GlslCodeGenerator::generate(const XorOp& op) {
	outRegisterEquals(op.destination);
	// XOR is currently only used to implement negation this is a hacky way to make it work.
	out() << "-";
	outRegisterName(op.lhs);
	out() << ";\n";
	/*out() << "uintBitsToFloat(";
	{
		out() << "floatBitsToUint(";
		outRegisterName(op.lhs);
		out() << ")";
	}
	out() << " ^ ";
	{
		out() << "floatBitsToUint(";
		outRegisterName(op.rhs);
		out() << ")";
	}
	out() << ");\n";*/
}

void GlslCodeGenerator::generate(const NegateOp& op) {
	outRegisterEquals(op.destination);
	out() << "-" << op.operand << '\n';
}

void GlslCodeGenerator::generate(const FunctionOp& op) {
	outRegisterEquals(op.destination);
	out() << op.functionName << "(";
	for (i32 i = 0; i < i32(op.arguments.size()) - 1; i++) {
		out() << op.arguments[i];
		out() << ", ";
	}
	if (op.arguments.size() > 0) {
		out() << op.arguments[op.arguments.size() - 1];
	}
	out() << ");\n";
}

void GlslCodeGenerator::generate(const ReturnOp& op) {
	out() << "result = ";
	outRegisterName(op.returnedRegister);
	out() << ";\n";
}

void GlslCodeGenerator::outRegisterName(Register reg) {
	out() << "r" << reg;
}

void GlslCodeGenerator::outRegisterEquals(Register reg) {
	outRegisterName(reg);
	out() << " = ";
}

void GlslCodeGenerator::outVariableName(i64 variableIndex) {
	// To prevent aliasing with reserved glsl variables and other variables use this as the name.
	out() << "v" << variableIndex;
}

void GlslCodeGenerator::outInfixBinaryOp(Register destination, Register lhs, Register rhs, const char* op) {
	outRegisterEquals(destination);
	outRegisterName(lhs);
	out() << ' ' << op << ' ';
	outRegisterName(rhs);
	out() << ";\n";
}

std::ostream& GlslCodeGenerator::out() {
	return *out_;
}
