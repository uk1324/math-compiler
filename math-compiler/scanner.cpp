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
	output.push_back(makeToken(TokenType::END_OF_FILE));

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

	case '(': return makeToken(TokenType::LEFT_PAREN);
	case ')': return makeToken(TokenType::RIGHT_PAREN);

	default:
		if (isDigit(c)) {
			return number();
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
		if (isDigit(peek())) {
			advance();
		} else {
			break;
		}
	}
	return makeToken(TokenType::FLOAT);
}

u8 Scanner::peek() {
	if (currentCharIndex >= source.size()) {
		return '\0';
	}
	ASSERT(currentCharIndex >= 0);
	return source[static_cast<usize>(currentCharIndex)];
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
	if (currentCharIndex < source.size()) {
		currentCharIndex++;
	}
}

bool Scanner::eof() {
	return currentCharIndex >= source.size();
}

Token Scanner::makeToken(TokenType type) {
	const auto length = currentCharIndex - currentTokenStartIndex;
	ASSERT_NOT_NEGATIVE(length);
	ASSERT_NOT_NEGATIVE(currentTokenStartIndex);
	Token token(type, source.substr(static_cast<usize>(currentTokenStartIndex), static_cast<usize>(length)));
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
