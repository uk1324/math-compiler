#pragma once

#include <string_view>

//struct FloatToken {};

//using Token = std::variant<FloatToken, >;

enum class TokenType {
	FLOAT,
	PLUS,
	MINUS,
	STAR,
	SLASH,
	LEFT_PAREN,
	RIGHT_PAREN,
	IDENTIFIER,
	END_OF_SOURCE,
	ERROR,
};

struct Token {
	Token(TokenType type, std::string_view source);

	bool operator==(const Token&) const = default;

	std::string_view source;
	TokenType type;
};

const char* tokenToStr(const Token& token);
