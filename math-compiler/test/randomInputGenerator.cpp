#include "randomInputGenerator.hpp"
#include "../utils/asserts.hpp"
//#include <random>


#define CHECK_NESTING() \
	if (nesting <= 0) { \
		constantExpr(nesting); \
		return; \
	}

RandomInputGenerator::RandomInputGenerator()
	: rng(dev()) {
}

std::string_view RandomInputGenerator::generate(std::span<const FunctionParameter> parameters) {
	this->parameters = parameters;
	out.string().clear();
	const auto nesting = randomFrom0To(maxNestingDepth);
	expr(nesting);
	return out.string();
}

void RandomInputGenerator::expr(i32 nesting) {
	binaryExpr(nesting);
}

void RandomInputGenerator::binaryExpr(i32 nesting) {
	CHECK_NESTING();
	const bool useParens = randomBool();

	whitespace();
	if (useParens) {
		out << '(';
		whitespace();
	}

	const char* op[] = { "+", "-", "*", "/" };

	unaryExpr(nesting - 1);

	whitespace();

	out << op[randomFrom0To(std::size(op))];

	whitespace();

	unaryExpr(nesting - 1);

	whitespace();

	if (useParens) {
		out << ')';
	}
}

void RandomInputGenerator::unaryExpr(i32 nesting) {
	const bool addOperation = randomBool();
	if (!addOperation) {
		primaryExpr(nesting);
		return;
	}

	const char op[] = "-";
	out << randomFrom0To(std::size(op));
	whitespace();
	primaryExpr(nesting - 1);
}

void RandomInputGenerator::primaryExpr(i32 nesting) {

	enum PrimaryExprType {
		PAREN,
		CONSTANT,
		IDENTIFIER,

		COUNT,
	} const type = PrimaryExprType(randomFrom0To(COUNT));

	switch (type) {
	case PAREN:
		parenExpr(nesting);
		break;

	case CONSTANT:
		constantExpr(nesting);
		break;

	case IDENTIFIER:
		identifierExpr(nesting);
		break;
	}

	const auto chainLength = randomFrom0To(maxImplicitMultiplicationChainLength);
	for (i32 i = 0; i < chainLength; i++) {
		enum ImplicitMulitplicationExprType {
			IDENTIFIER,
			PAREN,
			COUNT,
		} const type = ImplicitMulitplicationExprType(randomFrom0To(COUNT));

		switch (type) {
		case PAREN:
			parenExpr(nesting);
			break;

		case IDENTIFIER:
			identifierExpr(nesting);
			break;
		}
	}
}

void RandomInputGenerator::constantExpr(i32 nesting) {
	const auto integerPartLength = randomInRangeInclusive(1, integerPartMaxLength);
	ASSERT(integerPartLength > 0);

	for (i32 i = 0; i < integerPartLength; i++) {
		out << randomDigit();
	}

	bool fractionalPart = randomBool();
	if (!fractionalPart) {
		return;
	}
	out << ".";

	const auto fractionalPartLength = randomFrom0To(fractionalPartMaxLength);
	for (i32 i = 1; i < fractionalPartLength; i++) {
		out << randomDigit();
	}
}

void RandomInputGenerator::parenExpr(i32 nesting) {
	out << '(';
	
	whitespace();
	if (nesting <= 0) {
		constantExpr(nesting);
	} else {
		expr(nesting - 1);
	}
	whitespace();
	out << ')';
}

void RandomInputGenerator::identifierExpr(i32 nesting) {
	if (parameters.size() <= 0) {
		constantExpr(nesting);
		return;
	}
	// TODO: Maybe generate invalid variables.
	const auto name = parameters[randomFrom0To(parameters.size())].name;
	out << name;
	if (isdigit(name.back())) {
		out << ' ';
	}

}

void RandomInputGenerator::whitespace() {
	const auto length = randomFrom0To(whitespaceMaxLength);
	for (i32 i = 0; i < length; i++) {
		out << ' ';
	}
}

//i32 RandomInputGenerator::randomNumber() {
//	return rand();
//}

bool RandomInputGenerator::randomBool() {
	return std::uniform_int_distribution<int>(0, 1)(rng) == 0;
}

i32 RandomInputGenerator::randomFrom0To(i32 x) {
	return std::uniform_int_distribution<i32>(0, x - 1)(rng);
	//return randomNumber() % x;
}

i32 RandomInputGenerator::randomInRangeExclusive(i32 includingStart, i32 excludingEnd) {
	return std::uniform_int_distribution<i32>(includingStart, excludingEnd - 1)(rng);
}

i32 RandomInputGenerator::randomInRangeInclusive(i32 includingStart, i32 includingEnd) {
	return std::uniform_int_distribution<i32>(includingStart, includingEnd)(rng);
}

char RandomInputGenerator::randomDigit() {
	char digits[] = "1234567890";
	// -1 because of the null byte.
	return digits[randomFrom0To(std::size(digits) - 1)];
}
