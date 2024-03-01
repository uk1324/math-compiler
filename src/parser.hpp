#pragma once

#include "utils/ints.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "astAllocator.hpp"
#include "parserMessageReporter.hpp"
#include <vector>
#include <optional>

struct Parser {
	struct Error {};

	Parser();
	void initialize(
		const std::vector<Token>* tokens, 
		std::string_view source, 
		ParserMessageReporter* reporter);

	// TODO: Use span.
	std::optional<Ast> parse(
		const std::vector<Token>& tokens, 
		std::string_view source, 
		ParserMessageReporter& reporter);
	Expr* expr();
	Expr* binaryExpr();
	Expr* plusOrMinusBinaryExpr();
	Expr* timesOrDivideBinaryExpr();
	Expr* primaryExpr();
	Expr* function(std::string_view name, i64 start);

	const Token& peek();
	const Token& peekPrevious();
	bool check(TokenType type);
	bool match(TokenType type);
	void expect(TokenType type);
	void advance();
	bool eof();
	[[noreturn]] void throwError(const ParserError& error);

	std::string_view tokenSource(const Token& token) const;

	const std::vector<Token>* tokens;
	i64 currentTokenIndex;

	ParserMessageReporter* messageReporter;
	std::string_view source;
	AstAllocator astAllocator;
};