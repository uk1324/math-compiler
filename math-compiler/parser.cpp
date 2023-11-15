#include "parser.hpp"
#include "astAllocator.hpp"
#include "utils/asserts.hpp"
#include <charconv>

Parser::Parser() {
	initialize(nullptr, nullptr);
}

void Parser::initialize(const std::vector<Token>* tokens, ParserMessageReporter* reporter) {
	this->tokens = tokens;
	currentTokenIndex = 0;
	messageReporter = reporter;
}

std::optional<Ast> Parser::parse(const std::vector<Token>& tokens, ParserMessageReporter* reporter) {
	initialize(&tokens, reporter);

	try {
		const auto root = expr();
		if (peek().type == TokenType::ERROR) {
			// Could try to synchronize.
			return std::nullopt;
			// Unexpected peek().type.
			//ASSERT(peek().type == TokenType::END_OF_FILE);
		}
		return Ast{
			.root = root
		};
	} catch (const Error&) {
		return std::nullopt;
	}
}

Expr* Parser::expr() {
	return binaryExpr();
}

Expr* Parser::binaryExpr() {
	return plusOrMinusBinaryExpr();
}

Expr* Parser::plusOrMinusBinaryExpr() {
	auto lhs = timesOrDivideBinaryExpr();

	while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
		const auto operatorTokenType = peekPrevious().type;
		const auto rhs = timesOrDivideBinaryExpr();
		switch (operatorTokenType) {
		case TokenType::PLUS:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::PLUS);
			break;

		case TokenType::MINUS:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::MINUS);
			break;

		default:
			ASSERT_NOT_REACHED();
		}
	}

	return lhs;
}

Expr* Parser::timesOrDivideBinaryExpr() {
	auto lhs = primaryExpr();

	while (match(TokenType::STAR) || match(TokenType::SLASH)) {
		const auto operatorTokenType = peekPrevious().type;
		const auto rhs = primaryExpr();
		switch (operatorTokenType) {
		case TokenType::STAR:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::MULTIPLY);
			break;

		case TokenType::SLASH:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::DIVIDE);
			break;

		default:
			ASSERT_NOT_REACHED();
		}
	}
	
	return lhs;
}

Expr* Parser::primaryExpr() {
	if (match(TokenType::FLOAT)) {
		const auto numberTokenSource = peekPrevious().source;
		Real value;
		const auto result = std::from_chars(
			numberTokenSource.data(), 
			numberTokenSource.data() + numberTokenSource.length(), 
			value
		);
		if (result.ec != std::errc()) {
			// The scanner shouldn't have let throught a invalid number.
			ASSERT_NOT_REACHED();
		}
		return astAllocator.allocate<ConstantExpr>(value);
	} else if (match(TokenType::LEFT_PAREN)) {
		Expr* e = expr();
		if (!match(TokenType::RIGHT_PAREN)) {
			// unterminated paren. // give the position of the first paren.
		}
		return e;
	}
	throwError(UnexpectedTokenParserError{ .token = peek() });
}

const Token& Parser::peek() {
	ASSERT_NOT_NEGATIVE(currentTokenIndex);
	return (*tokens)[static_cast<usize>(currentTokenIndex)];
}

const Token& Parser::peekPrevious() {
	ASSERT(currentTokenIndex > 0);
	return (*tokens)[static_cast<usize>(currentTokenIndex) - 1];
}

bool Parser::match(TokenType type) {
	if (peek().type == type) {
		advance();
		return true;
	}
	return false;
}

void Parser::advance() {
	ASSERT_NOT_NEGATIVE(currentTokenIndex);
	if ((*tokens)[static_cast<usize>(currentTokenIndex)].type == TokenType::END_OF_FILE) {
		return;
	}
	currentTokenIndex++;
}

void Parser::throwError(const ParserError& error) {
	messageReporter->onError(error);
	throw Error();
}