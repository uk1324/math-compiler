#pragma once

#include "utils/ints.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "astAllocator.hpp"
#include "parserMessageReporter.hpp"
#include <vector>
#include <optional>

struct Parser {
	struct Error {

	};

	Parser();
	void initialize(const std::vector<Token>* tokens, ParserMessageReporter* reporter);

	// TODO: Use span.
	std::optional<Ast> parse(const std::vector<Token>& tokens, ParserMessageReporter* reporter);
	Expr* expr();
	Expr* binaryExpr();
	Expr* plusOrMinusBinaryExpr();
	Expr* timesOrDivideBinaryExpr();
	Expr* primaryExpr();

	const Token& peek();
	const Token& peekPrevious();
	bool check(TokenType type);
	bool match(TokenType type);
	void expect(TokenType type);
	void advance();
	void throwError(const ParserError& error);

	const std::vector<Token>* tokens;
	i64 currentTokenIndex;

	ParserMessageReporter* messageReporter;
	AstAllocator astAllocator;
};