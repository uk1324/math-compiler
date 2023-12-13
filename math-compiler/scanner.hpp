#pragma once

#include "utils/ints.hpp"
#include "scannerMessageReporter.hpp"
#include "ast.hpp"
#include <vector>

struct Scanner {
	struct Error {

	};

	Scanner();

	void initialize(std::string_view source, ScannerMessageReporter* reporter);

	std::vector<Token> parse(std::string_view source, ScannerMessageReporter* reporter);

	Token token();
	Token number();

	u8 peek();
	bool match(char c);
	void skipWhitespace();
	void advance();
	bool eof();
	Token makeToken(TokenType type);
	Token error(const ScannerError& error);

	static bool isDigit(u8 c);

	i64 currentTokenStartIndex;
	i64 currentCharIndex;
	std::string_view source;
	
	ScannerMessageReporter* messageReporter;
};