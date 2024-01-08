#include "token.hpp"
#include "utils/asserts.hpp"

Token::Token(TokenType type, std::string_view source)
	: type(type)
	, source(source) {}

const char* tokenToStr(const Token& token) {
	switch (token.type) {
	case TokenType::PLUS: return "PLUS";
	case TokenType::MINUS: return "MINUS";
	case TokenType::FLOAT: return "FLOAT";
	case TokenType::STAR: return "STAR";
	case TokenType::SLASH: return "SLASH";
	case TokenType::LEFT_PAREN: return "LEFT_PAREN";
	case TokenType::RIGHT_PAREN: return "LEFT_PAREN";
	case TokenType::END_OF_SOURCE: return "END_OF_SOURCE";
	case TokenType::IDENTIFIER: return "IDENTIFIER";
	case TokenType::ERROR: return "ERROR";
	}
	ASSERT_NOT_REACHED();
	return nullptr;
}
