#include "parser.hpp"
#include "astAllocator.hpp"
#include "input.hpp"
#include "utils/asserts.hpp"
#include <charconv>

Parser::Parser() {
	initialize(nullptr, std::string_view(), nullptr);
}

void Parser::initialize(
	const std::vector<Token>* tokens, 
	std::string_view source, 
	ParserMessageReporter* reporter) {
	this->tokens = tokens;
	this->source = source;
	currentTokenIndex = 0;
	messageReporter = reporter;
	astAllocator.reset();
}

std::optional<Ast> Parser::parse(
	const std::vector<Token>& tokens, 
	std::string_view source, 
	ParserMessageReporter& reporter) {
	initialize(&tokens, source, &reporter);

	try {
		const auto root = expr();
		if (peek().type == TokenType::ERROR) {
			// Could try to synchronize.
			return std::nullopt;
			// Unexpected peek().type.
			//ASSERT(peek().type == TokenType::END_OF_FILE);
		} else if (peek().type != TokenType::END_OF_SOURCE) {
			reporter.onError(UnexpectedTokenParserError{ .token = peek() });
			return std::nullopt;
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
	const auto start = peek().start();
	auto lhs = timesOrDivideBinaryExpr();

	while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
		const auto operatorTokenType = peekPrevious().type;
		const auto rhs = timesOrDivideBinaryExpr();
		const auto end = peek().end();
		switch (operatorTokenType) {
		case TokenType::PLUS:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::ADD, start, end);
			break;

		case TokenType::MINUS:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::SUBTRACT, start, end);
			break;

		case TokenType::STAR:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::MULTIPLY, start, end);
			break;

		case TokenType::SLASH:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::DIVIDE, start, end);
			break;

		default:
			ASSERT_NOT_REACHED();
		}
	}

	return lhs;
}

Expr* Parser::timesOrDivideBinaryExpr() {
	const auto start = peek().start();
	auto lhs = primaryExpr();

	while (match(TokenType::STAR) || match(TokenType::SLASH)) {
		const auto operatorTokenType = peekPrevious().type;
		const auto rhs = primaryExpr();
		const auto end = peek().end();
		switch (operatorTokenType) {
		case TokenType::STAR:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::MULTIPLY, start, end);
			break;

		case TokenType::SLASH:
			lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::DIVIDE, start, end);
			break;

		default:
			ASSERT_NOT_REACHED();
		}
	}
	
	return lhs;
}

Expr* Parser::primaryExpr() {
	Expr* lhs = nullptr;

	auto parenExprAfterMatch = [this]() -> Expr* {
		auto e = expr();
		expect(TokenType::RIGHT_PAREN);
		return e;
	};

	i64 lhsStart;

	if (match(TokenType::FLOAT)) {
		const auto& numberToken = peekPrevious();
		const auto numberTokenSource = tokenSource(numberToken);
		Real value;
		const auto result = std::from_chars(
			numberTokenSource.data(), 
			numberTokenSource.data() + numberTokenSource.length(), 
			value
		);
		if (result.ec != std::errc()) {
			// The scanner shouldn't have let throught a invalid number.
			ASSERT_NOT_REACHED();
			return nullptr;
		}
		
		lhs = astAllocator.allocate<ConstantExpr>(value, numberToken.start(), numberToken.end());
		lhsStart = numberToken.start();

		if (match(TokenType::CARET)) {
			return exponentiationExpr(lhs, lhsStart);
		}

	} else if (match(TokenType::LEFT_PAREN)) {
		lhsStart = peek().start();
		lhs = parenExprAfterMatch();
		if (match(TokenType::CARET)) {
			return exponentiationExpr(lhs, lhsStart);
		}
	} else if (match(TokenType::VARIABLE)) {
		const auto& identifierToken = peekPrevious();
		lhsStart = identifierToken.start();
		const auto identifier = tokenSource(identifierToken);
		lhs = astAllocator.allocate<IdentifierExpr>(
			identifier,
			lhsStart,
			identifierToken.end());

		if (match(TokenType::CARET)) {
			return exponentiationExpr(lhs, lhsStart);
		}

	} else if (match(TokenType::FUNCTION)) {
		lhsStart = peekPrevious().start();
		lhs = function(tokenSource(peekPrevious()), lhsStart);

		if (match(TokenType::CARET)) {
			return exponentiationExpr(lhs, lhsStart);
		}
	} else if (match(TokenType::MINUS)) {
		// TODO: Not sure if this should be changed but -4x will parse to -(4 * x) and not (-4) * x. (I wrote this when I thought that the order is reversed but I guess -(4 * x) makes more sense thatn the other option. Don't think that will change the result but not sure. GCC treats the differently. I guess if x is NaN then the result might have different signs idk.
		const auto start = peekPrevious().start();
		const auto& operand = primaryExpr();
		const auto end = peek().end();
		return astAllocator.allocate<UnaryExpr>(
			operand,
			UnaryOpType::NEGATE,
			start,
			end
		);
	} else {
		throwError(UnexpectedTokenParserError{ .token = peek() });
	}

	for (;;) {
		i64 rhsStart;
		i64 rhsEnd;

		Expr* rhs;
		if (match(TokenType::VARIABLE)) {
			const auto& identifierToken = peekPrevious();
			const auto identifier = tokenSource(identifierToken);
			const auto start = identifierToken.start();
			rhsStart = start;
			rhs = astAllocator.allocate<IdentifierExpr>(
				identifier,
				start,
				identifierToken.end());
			rhsEnd = identifierToken.end();
		} else if (match(TokenType::FUNCTION)) {
			rhsStart = peekPrevious().start();
			rhs = function(tokenSource(peekPrevious()), rhsStart);
			rhsEnd = peekPrevious().end();
		} else if (match(TokenType::LEFT_PAREN)) {
			rhsStart = peekPrevious().start();
			rhs = parenExprAfterMatch();
			rhsEnd = peekPrevious().end();
		} else {
			break;
		}

		if (match(TokenType::CARET)) {
			rhs = exponentiationExpr(rhs, rhsStart);
			return astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::MULTIPLY, lhsStart, rhsEnd);
		}

		lhs = astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::MULTIPLY, lhsStart, rhsEnd);
	}

	return lhs;
}

Expr* Parser::exponentiationExpr(Expr* lhs, i64 start) {
	const auto rhs = primaryExpr();
	const auto end = peekPrevious().end();
	return astAllocator.allocate<BinaryExpr>(lhs, rhs, BinaryOpType::EXPONENTIATE, start, end);
}


Expr* Parser::function(std::string_view name, i64 start) {
	// TODO: Maybe make a new error expected left paren after function name.
	expect(TokenType::LEFT_PAREN);

	if (match(TokenType::RIGHT_PAREN)) {
		return astAllocator.allocate<FunctionExpr>(name, std::span<const Expr*>(), start, peek().end());
	}

	AstAllocator::List<const Expr*> arguments;
	while (!eof()) {
		astAllocator.listAppend(arguments, (const Expr*)(expr()));
		if (match(TokenType::RIGHT_PAREN)) {
			break;
		}
		expect(TokenType::COMMA);
		if (match(TokenType::RIGHT_PAREN)) {
			break;
		}
	}

	return astAllocator.allocate<FunctionExpr>(name, arguments.span(), start, peek().end());
}

const Token& Parser::peek() {
	ASSERT_NOT_NEGATIVE(currentTokenIndex);
	return (*tokens)[static_cast<usize>(currentTokenIndex)];
}

const Token& Parser::peekPrevious() {
	ASSERT(currentTokenIndex > 0);
	return (*tokens)[static_cast<usize>(currentTokenIndex) - 1];
}

bool Parser::check(TokenType type) {
	return peek().type == type;
}

bool Parser::match(TokenType type) {
	if (peek().type == type) {
		advance();
		return true;
	}
	return false;
}

void Parser::expect(TokenType type) {
	if (match(type)) {
		return;
	}
	throwError(ExpectedTokenParserError{ .expected = type, .found = peek() });
}

void Parser::advance() {
	ASSERT_NOT_NEGATIVE(currentTokenIndex);
	if ((*tokens)[static_cast<usize>(currentTokenIndex)].type == TokenType::END_OF_SOURCE) {
		return;
	}
	currentTokenIndex++;
}

bool Parser::eof() {
	return peek().type == TokenType::END_OF_SOURCE;
}

void Parser::throwError(const ParserError& error) {
	messageReporter->onError(error);
	throw Error();
}

std::string_view Parser::tokenSource(const Token& token) const {
	return source.substr(token.location.start, token.location.length);
}
