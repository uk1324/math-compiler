#include "printAst.hpp"
#include "utils/asserts.hpp"
#include "utils/put.hpp"

void printBinaryOpType(BinaryOpType type) {
	switch (type)
	{
	case BinaryOpType::ADD:
		std::cout << "+";
		break;
	case BinaryOpType::SUBTRACT:
		std::cout << "-";
		break;
	case BinaryOpType::MULTIPLY:
		std::cout << "*";
		break;
	case BinaryOpType::DIVIDE:
		std::cout << "/";
		break;
	default:
		ASSERT_NOT_REACHED();
		break;
	}
}

void printExpr(const Expr* e, bool printExtraParens) {
	switch (e->type)
	{
	case ExprType::BINARY: {
		const auto binaryExpr = static_cast<const BinaryExpr*>(e);
		if (printExtraParens) {
			std::cout << "(";
		}
		printExpr(binaryExpr->lhs, printExtraParens);
		std::cout << " ";
		printBinaryOpType(binaryExpr->op);
		std::cout << " ";
		printExpr(binaryExpr->rhs, printExtraParens);
		if (printExtraParens) {
			std::cout << ")";
		}
		break;
	}

	case ExprType::UNARY: {
		const auto unaryExpr = static_cast<const UnaryExpr*>(e);
		if (printExtraParens) {
			put("(");
		}

		switch (unaryExpr->op) {
			using enum UnaryOpType;

		case NEGATE:
			put("-");
			printExpr(unaryExpr->operand, printExtraParens);
			break;
		}

		if (printExtraParens) {
			put(")");
		}
		break;
	}
		

	case ExprType::CONSTANT: {
		const auto constantExpr = static_cast<const ConstantExpr*>(e);
		std::cout << constantExpr->value;
		break;
	}

	case ExprType::IDENTIFIER: {
		const auto identifierExpr = static_cast<const IdentifierExpr*>(e);
		putnn("%", identifierExpr->identifier);
		break;
	}

	case ExprType::FUNCTION: {
		const auto functionExpr = static_cast<const FunctionExpr*>(e);
		putnn("%(", functionExpr->functionName);
		for (i64 i = 0; i < i64(functionExpr->arguments.size()) - 1; i++) {
			printExpr(functionExpr->arguments[i], printExtraParens);
		}
		putnn(")");
		break;
	}

	default:
		ASSERT_NOT_REACHED();
		break;
	}
}
