#pragma once

#include "utils/ints.hpp"
#include "token.hpp"
#include <variant>

struct UnexpectedTokenParserError {
	// TODO: Fix the error if you change this to a reference. It probably has something to do with the comparasion operator.
	// If you change this to Token& then 
	// UnexpectedTokenParserError{ .token = token } == UnexpectedTokenParserError{ .token = token };
	// gives an error.
	const Token token;

	bool operator==(const UnexpectedTokenParserError&) const = default;
};

struct ExpectedTokenParserError {
	const TokenType expected;
	const Token found;

	bool operator==(const ExpectedTokenParserError&) const = default;
};

//struct UnterminatedBracketsParserError {
//	u8 bracketChar;
//	i64 firstBracketIndex;
//	Token found;
//};

using ParserError = std::variant<UnexpectedTokenParserError, ExpectedTokenParserError>;

// Maybe rename message handler.
struct ParserMessageReporter {
	virtual void onError(const ParserError& error) = 0;
};