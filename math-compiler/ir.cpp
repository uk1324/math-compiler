#include "ir.hpp"
#include "utils/overloaded.hpp"
#include "utils/put.hpp"

void printBinaryOp(std::ostream& out, const char* opName, Register lhs, Register rhs, Register destination) {
	out << opName << " r" << destination << " <- r" << lhs << " r" << rhs << '\n';
}

bool registerIsParamter(std::span<const FunctionParameter> parameters, Register r) {
	return r < i64(parameters.size());
}

void printIrOp(std::ostream& out, const IrOp& op) {
	std::visit(overloaded{
		[&](const LoadConstantOp& load) {
			put("load r% <- '%'", load.destination, load.constant);
		},
		[&](const AddOp& add) {
			printBinaryOp(out, "add", add.lhs, add.rhs, add.destination);
		},
		[&](const SubtractOp& add) {
			printBinaryOp(out, "sub", add.lhs, add.rhs, add.destination);
		},
		[&](const MultiplyOp& add) {
			printBinaryOp(out, "mul", add.lhs, add.rhs, add.destination);
		},
		[&](const DivideOp& add) {
			printBinaryOp(out, "div", add.lhs, add.rhs, add.destination);
		},
		[&](const XorOp& add) {
			printBinaryOp(out, "xor", add.lhs, add.rhs, add.destination);
		},
		[&](const NegateOp& op) {
			put("neg r% <- r%", op.destination, op.operand);
		},
		[&](const FunctionOp& op) {
			putnn("call r%, <- %(", op.destination, op.functionName);
			if (op.arguments.size() == 0) {
				put(")");
				return;
			}
			for (i64 i = 0; i < i64(op.arguments.size()) - 1; i++) {
				putnn("r%, ", op.arguments[i]);
			}
			put("r%)", op.arguments.back());
		},
		[&](const ReturnOp& ret) {
			out << "ret r" << ret.returnedRegister << '\n';
		}
	}, op);
}

void printIrCode(std::ostream& out, const std::vector<IrOp>& code) {
	for (const auto& op : code) {
		printIrOp(out, op);
	}
}
