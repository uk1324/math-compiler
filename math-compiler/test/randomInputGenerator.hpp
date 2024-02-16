#pragma once

#include "../utils/stringStream.hpp"
#include "../input.hpp"
#include "../utils/ints.hpp"
#include <span>
#include <random>

struct RandomInputGenerator {
	RandomInputGenerator();

	std::string_view generate(
		std::span<const FunctionParameter> parameters,
		std::span<const FunctionInfo> functions);

	void expr(i32 nesting);
	void binaryExpr(i32 nesting);
	void unaryExpr(i32 nesting);
	void primaryExpr(i32 nesting);
	void constantExpr(i32 nesting);
	void parenExpr(i32 nesting);
	void identifierExpr(i32 nesting);
	void functionExpr(i32 nesting);
	
	// Whitespace should be generated after the first character and before the last.
	// For example this is wrong: '  (a + b)' but this is not '(a   +b )'.
	void whitespace();

	//i32 randomNumber();
	bool randomBool();
	i32 randomFrom0To(i32 x);
	i32 randomInRangeExclusive(i32 includingStart, i32 excludingEnd);
	i32 randomInRangeInclusive(i32 includingStart, i32 includingEnd);
	char randomDigit();

	i32 maxNestingDepth = 5;
	i32 whitespaceMaxLength = 3;
	i32 integerPartMaxLength = 5;
	i32 fractionalPartMaxLength = 5;
	i32 maxImplicitMultiplicationChainLength = 5;

	std::random_device dev;
	std::mt19937 rng;

	StringStream out;
	std::span<const FunctionParameter> parameters;
	std::span<const FunctionInfo> functions;
};