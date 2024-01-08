#include "printAst.hpp"
#include "utils/asserts.hpp"
#include "utils/put.hpp"

void printBinaryOpType(BinaryOpType type) {
	switch (type)
	{
	case BinaryOpType::PLUS:
		std::cout << "+";
		break;
	case BinaryOpType::MINUS:
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

void printExpr(Expr* e, bool printExtraParens) {
	switch (e->type)
	{
	case ExprType::BINARY: {
		const auto binaryExpr = static_cast<BinaryExpr*>(e);
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

	case ExprType::CONSTANT: {
		const auto constantExpr = static_cast<ConstantExpr*>(e);
		std::cout << constantExpr->value;
		break;
	}

	case ExprType::IDENTIFIER: {
		const auto identifierExpr = static_cast<IdentifierExpr*>(e);
		putnn("%", identifierExpr->identifier);
		break;
	}

	default:
		ASSERT_NOT_REACHED();
		break;
	}
}
