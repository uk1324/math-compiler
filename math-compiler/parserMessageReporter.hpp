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

using ParserError = std::variant<UnexpectedTokenParserError>;

// Maybe rename message handler.
struct ParserMessageReporter {
	virtual void onError(const ParserError& error) = 0;
};