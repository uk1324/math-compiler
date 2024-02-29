#pragma once

#include <string_view>
#include <immintrin.h>
#include "utils/ints.hpp"
#include "scanner.hpp"
#include "parser.hpp"
#include "irCompiler.hpp"
#include "codeGenerator.hpp"
//#include "machineCode.hpp"

struct LoopFunctionArray {
	LoopFunctionArray(i64 valuesPerBlock);
	void append(std::span<const float> block);

	i64 valuesPerBlock;
	i64 itemCount;

	__m256* data;
	i64 dataCapacity;
};

struct Runtime {
	struct LoopFunction {
		LoopFunction(const MachineCode& machineCode);
		LoopFunction(LoopFunction&& other) noexcept;
		LoopFunction(const LoopFunction&) = delete;
		~LoopFunction();
		void operator()(const __m256* input, __m256* output, i64 count);

		using Function = void (*)(const __m256*, __m256*, i64);
		Function function;
		// TODO: Maybe store capacity so the function can be reallocated.
	};

	Runtime(
		ScannerMessageReporter& scannerReporter,
		ParserMessageReporter& parserReporter,
		IrCompilerMessageReporter& irCompilerReporter);

	std::optional<LoopFunction> compileFunction(
		std::string_view source, 
		std::span<const Variable> variables);
	
	Scanner scanner;
	Parser parser;
	IrCompiler compiler;
	CodeGenerator codeGenerator;

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