#include <iostream>
#include "scanner.hpp"
#include "parser.hpp"
#include "utils/asserts.hpp"
#include "utils/fileIo.hpp"
#include "printAst.hpp"
#include "evaluateAst.hpp"
#include "utils/pritningUtils.hpp"
#include "ostreamScannerMessageReporter.hpp"
#include "ostreamParserMessageReporter.hpp"
#include "ostreamIrCompilerMessageReporter.hpp"
#include "utils/put.hpp"
#include "irCompiler.hpp"
#include "irVm.hpp"
#include "codeGenerator.hpp"
#include "debug.hpp"
#include "valueNumbering.hpp"
#include "deadCodeElimination.hpp"
#include "executeFunction.hpp"
#include "simdFunctions.hpp"
#include "os/os.hpp"
#include "test/fuzzTests.hpp"
#include "runtimeUtils.hpp"
#include "test/tests.hpp"
#include "test/floatingPointIndentityTest.hpp"
#include <span>
#include <sstream>
#include <fstream>
#include <filesystem>

// TODO: Replace normal floating point equals with bitwise equals. !!!!!
void test() {

	//std::string_view source = "exp(2) + exp(x)";
	/*std::string_view source = "  exp(0)  + -  13.x_3 exp  (  exp(  (9217.680) )  ((475.925)  )exp  (  (4032.940))   )  )exp  (  (-  exp( (35.7))    ( (5320.94)  )exp  ( (21113) )  ( (5) ) -  -  14995.x_1  ) )";*/
	std::string_view source = "x_0";
	//std::string_view source = "x";
	//std::string_view source = "2";
	//FunctionParameter parameters[] { { "x_0" }, { "x_1" }, { "x_2" } };
	Variable parameters[] { { "x_0" }, { "x_1" }, { "x_2" }, { "x_3" }, { "x_4" } };
	float arguments[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	//float arguments[] = { std::bit_cast<float>(0xabb71ac3), std::bit_cast<float>(0x6f1139fd), std::bit_cast<float>(0x1da0a111), std::bit_cast<float>(0xc35446f0), std::bit_cast<float>(0x8d6f690), };

	std::ostream& outputStream = std::cerr;

	const FunctionInfo functionArray[] = {
		{ .name = "exp", .arity = 1, .address = expSimd, }
	};

	std::span<const FunctionInfo> functions = functionArray;

	OstreamScannerMessageReporter scannerReporter(outputStream, source);
	Scanner scanner;
	const auto& tokens = scanner.parse(source, functions, parameters, scannerReporter);
	//debugOutputTokens(tokens, source);

	OstreamParserMessageReporter parserReporter(outputStream, source);
	Parser parser;
	const auto ast = parser.parse(tokens, source, parserReporter);
	if (ast.has_value()) {
		printExpr(ast->root, true);
		const auto outputRes = evaluateAst(ast->root, parameters, functions, arguments);
		if (outputRes.isOk()) {
			put(" = %", outputRes.ok());
		} else {
			put("\nevaluation error: %", outputRes.err());
		}
	} else {
		put("parser error");
		return;
	}

	OstreamIrCompilerMessageReporter compilerReporter(outputStream, source);
	IrCompiler compiler;
	const auto irCode = compiler.compile(*ast, parameters, functions, compilerReporter);
	if (irCode.has_value()) {
		put("original");
		printIrCode(std::cout, *irCode);
	} else {
		put("ir compiler error");
		return;
	}
	
	LocalValueNumbering valueNumbering;
	auto optimizedCode = valueNumbering.run(*irCode, parameters);
	const auto copy = optimizedCode;
	DeadCodeElimination deadCodeElimination;
	deadCodeElimination.run(copy, parameters, optimizedCode);
	//const auto optimizedCode = **irCode;

	put("optimized");
	printIrCode(std::cout, optimizedCode);

	IrVm vm;
	const auto irVmOutput = vm.execute(optimizedCode, arguments, functions);
	if (irVmOutput.isOk()) {
		put("ir vm output: %", irVmOutput.ok());
	}

	CodeGenerator codeGenerator;
	const auto& machineCode = codeGenerator.compile(optimizedCode, functions, parameters);

	outputToFile("test.bin", machineCode.code);

	const auto out = executeFunction(machineCode, arguments);
	put("out = %", out);

	/*bin.write(reinterpret_cast<const char*>(buffer), machineCode.size());*/
}

#include "test/simdFunctionsTest.hpp"

void allocatorTest() {
	std::string_view source = "  (  (   -6185.9x_1 x_1 ((0) )/ 858exp  (  (157.) )  x_2  )(  ( (76) )x_1  -- exp(  (69.0)) x_2 (  (68755))  )exp ( ( 0. +-57 )  )   / - x_2 exp ((exp(  (5))      - - exp(  (5.64)  ) x_1 x_1  )  )x_3   )";
	//std::string_view source = "exp(2) + exp(x_0)";

	std::ostream& outputStream = std::cout;
	OstreamScannerMessageReporter scannerReporter(outputStream, source);
	Scanner scanner;
	OstreamParserMessageReporter parserReporter(outputStream, source);
	Parser parser;
	OstreamIrCompilerMessageReporter compilerReporter(outputStream, source);
	IrCompiler compiler;
	CodeGenerator codeGenerator;

	Variable parameters[]{ { "x_0" }, { "x_1" }, { "x_2" }, { "x_3" }, { "x_4" } };
	float arguments[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
	const FunctionInfo functions[] = {
		{ .name = "exp", .arity = 1, .address = expSimd, }
	};

	for (i64 i = 0; i < 1000000; i++) {
		const auto& tokens = scanner.parse(source, functions, parameters, scannerReporter);
		const auto ast = parser.parse(tokens, source, parserReporter);
		/*const auto irCode = compiler.compile(*ast, parameters, functions, compilerReporter);
		const auto& machineCode = codeGenerator.compile(irCode, functions, parameters);*/
	}
}

// https://stackoverflow.com/questions/4911993/how-to-generate-and-run-native-code-dynamically
int main(void) {
	//allocatorTest();
	test();
	//runFuzzTests();
	//testFloatingPointIdentites();
	//runTests();
	//testSimdFunctions();
}