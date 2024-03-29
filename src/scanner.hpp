#pragma once

#include "utils/ints.hpp"
#include "scannerMessageReporter.hpp"
#include "input.hpp"
#include "ast.hpp"
#include <vector>

struct Scanner {
	struct Error {

	};

	Scanner();

	void initialize(
		std::string_view source, 
		std::span<const FunctionInfo> functions,
		std::span<const Variable> variables,
		ScannerMessageReporter* reporter);

	const std::vector<Token>& parse(
		std::string_view source, 
		std::span<const FunctionInfo> functions,
		std::span<const Variable> variables,
		ScannerMessageReporter& reporter);

	Token token();
	Token number();
	Token identifier(u8 firstChar);

	u8 peek();
	bool match(char c);
	void skipWhitespace();
	void advance();
	bool eof();
	Token makeToken(TokenType type);
	Token error(const ScannerError& error);

	static bool isDigit(u8 c);
	static bool isAlpha(u8 c);

	std::vector<Token> tokens;

	i64 currentTokenStartIndex;
	i64 currentCharIndex;
	std::string_view source;
	
	std::span<const FunctionInfo> functions;
	std::span<const Variable> variables;

	ScannerMessageReporter* messageReporter;
};