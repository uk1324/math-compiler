#pragma once

#include <string_view>
#include <immintrin.h>
#include "utils/ints.hpp"
#include "irCompiler.hpp"
#include "scanner.hpp"
#include "parser.hpp"

struct LoopFunctionArray {
	void add(std::span<const float> values);

	__m256* data;

	i32 valuesPerBatch;
	i64 length;
};

struct Runtime {
	using LoopFunction = __m256 (*)(__m256* input, __m256* output, i64 count);

	Runtime(
		ScannerMessageReporter& scannerReporter,
		ParserMessageReporter& parserReporter,
		IrCompilerMessageReporter& irCompilerReporter);

	LoopFunction compileFunction(
		std::string_view name, 
		std::string_view source, 
		std::span<const FunctionParameter> parameters);
	
	Scanner scanner;
	Parser parser;

	/*using LoopFunction = __m256 (*)(__m256* input, __m256* output, i64 count);

	LoopFunction addFunction(std::string_view name, std::string_view source, std::span<const FunctionParameter> parameters);
	using UnarySimdFunction = __m256 (*)(__m256);
	void addFunction(std::string_view name, UnarySimdFunction function);*/

	std::vector<FunctionInfo> functions;

	//struct Function {
	//	void* function;
	//	// TODO: This could probably be moved to a common data allocator. One issue is that then the pointers shouldn't change.
	//	std::vector<u8> data;
	//	i32 arity;
	//	// TODO: For function inlining
	//	//std::vector<IrOp> irCode;
	//};

	ScannerMessageReporter& scannerReporter;
	ParserMessageReporter& parserReporter;
	IrCompilerMessageReporter& irCompilerReporter;
};