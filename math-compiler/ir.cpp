#include "ir.hpp"
#include "utils/overloaded.hpp"

void printBinaryOp(std::ostream& out, const char* opName, Register lhs, Register rhs, Register destination) {
	out << opName << " r" << lhs << " r" << rhs << " -> r" << destination << '\n';
}

void printIrOp(std::ostream& out, const IrOp& op) {
	std::visit(overloaded{
		[&](const LoadConstantOp& load) {
			out << "load " << " '" << load.constant << "' -> r" << load.destination << '\n';
		},
		[&](const AddOp& add) {
			printBinaryOp(out, "add", add.lhs, add.rhs, add.destination);
		},
		[&](const MultiplyOp& add) {
			printBinaryOp(out, "mul", add.lhs, add.rhs, add.destination);
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
