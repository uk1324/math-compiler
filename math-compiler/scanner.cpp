#include "scanner.hpp"
#include "utils/asserts.hpp"
#include <charconv>

Scanner::Scanner() {
	initialize("", nullptr);
}

void Scanner::initialize(std::string_view source, ScannerMessageReporter* reporter) {
	currentCharIndex = 0;
	currentTokenStartIndex = 0;
	this->source = source;
	this->messageReporter = reporter;
}

std::vector<Token> Scanner::parse(std::string_view source, ScannerMessageReporter* reporter) {
	std::vector<Token> output;

	initialize(source, reporter);

	while (!eof()) {
		try {
			skipWhitespace();
			if (eof()) {
				break;
			}
			output.push_back(token());
		} catch (const Error&)  {

		}
	}
	currentTokenStartIndex = currentCharIndex;
	output.push_back(makeToken(TokenType::END_OF_SOURCE));

	return output;
}

Token Scanner::token() {
	u8 c = peek();
	advance();

	switch (c) {
	case '+': return makeToken(TokenType::PLUS);
	case '-': return makeToken(TokenType::MINUS);
	case '*': return makeToken(TokenType::STAR);
	case '/': return makeToken(TokenType::SLASH);

	case ',': return makeToken(TokenType::COMMA);

	case '(': return makeToken(TokenType::LEFT_PAREN);
	case ')': return makeToken(TokenType::RIGHT_PAREN);

	default:
		// TODO: Should numbers like 0001 be allowed?
		if (isDigit(c)) {
			return number();
		}
		if (isAlpha(c)) {
			return identifier(c);
		}
		break;
	}
	// currentCharIndex should be >0 because was called.
	ASSERT_NOT_NEGATIVE(currentCharIndex);
	return error(IllegalCharScannerError{
		.character = c,
		.sourceOffset = currentCharIndex - 1
	});
}

Token Scanner::number() {
	while (!eof()) {
		if (match('.')) {
			while (!eof() && isDigit(peek())) {
				advance();
			}
			break;
		} else if (!isDigit(peek())) {
			break;
		}
		advance();
	}
	return makeToken(TokenType::FLOAT);
}

Token Scanner::identifier(u8 firstChar) {
	if (!match('_')) {
		return makeToken(TokenType::IDENTIFIER);
	}

	while (!eof() && (isAlpha(peek()) || isDigit(peek()))) {
		advance();
	}

	return makeToken(TokenType::IDENTIFIER);
}

u8 Scanner::peek() {
	if (currentCharIndex >= static_cast<i64>(source.size())) {
		return '\0';
	}
	ASSERT(currentCharIndex >= 0);
	return source[static_cast<usize>(currentCharIndex)];
}

bool Scanner::match(char c) {
	if (peek() == c) {
		advance();
		return true;
	}
	return false;
}

void Scanner::skipWhitespace() {
	while (!eof()) {
		const auto c = peek();
		switch (c) {
		case ' ':
			advance();
			break;

		default:
			currentTokenStartIndex = currentCharIndex;
			return;
		}
	}
}

void Scanner::advance() {
	if (currentCharIndex < static_cast<i64>(source.size())) {
		currentCharIndex++;
	}
}

bool Scanner::eof() {
	return currentCharIndex >= static_cast<i64>(source.size());
}

#include "utils/put.hpp"

Token Scanner::makeToken(TokenType type) {
	Token token(type, SourceLocation::fromStartEnd(currentTokenStartIndex, currentCharIndex));
	//put("'%'", source.substr(currentTokenStartIndex, currentCharIndex - currentTokenStartIndex));
	currentTokenStartIndex = currentCharIndex;
	return token;
}

Token Scanner::error(const ScannerError& error) {
	// What part of the source is this going to occupy.
	messageReporter->onError(error);
	return makeToken(TokenType::ERROR);
}

bool Scanner::isDigit(u8 c) {
	return c >= '0' && c <= '9';
}

bool Scanner::isAlpha(u8 c) {
	return (c >= 'a' && c >= 'z') || (c >= 'A' && c >= 'Z');
}
