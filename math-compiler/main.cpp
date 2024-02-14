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
#include "valueNumbering.hpp"
#include "deadCodeElimination.hpp"
#include "executeFunction.hpp"
#include "simdFunctions.hpp"
#include "os/os.hpp"
#include "test/fuzzTests.hpp"
#include "test/tests.hpp"
#include "test/floatingPointIndentityTest.hpp"
#include <span>
#include <sstream>
#include <fstream>
#include <filesystem>

void debugOutputToken(const Token& token, std::string_view originalSource) {
	const auto end = token.location.start + token.location.length;
	const auto tokenSource = originalSource.substr(token.location.start, token.location.length);
	put("% % = % '%'", token.location.start, end, tokenTypeToStr(token.type), tokenSource);
}
// TODO: Replace normal floating point equals with bitwise equals. !!!!!
void test() {

	std::string_view source = "2 + 2";
	//FunctionParameter parameters[] { { "x" }, { "y" }, { "z" } };
	FunctionParameter parameters[] { { "x_0" }, { "x_1" }, { "x_2" }, { "x_3" }, { "x_4" } };
	float arguments[] = { std::bit_cast<float>(0xf8e665f4), std::bit_cast<float>(0xff954a4a), std::bit_cast<float>(0x8d8a0542), std::bit_cast<float>(0x9b902791), std::bit_cast<float>(0x978140ff), };
	std::span<const std::string_view> functionNames;

	std::ostream& outputStream = std::cerr;

	FunctionVariable functions[] = {
		{ .name = "exp", .arity = 1, .address = expSimd, }
	};

	OstreamScannerMessageReporter scannerReporter(outputStream, source);
	Scanner scanner;
	const auto tokens = scanner.parse(source, &scannerReporter);
	//for (auto& token : tokens) {
	//	debugOutputToken(token, source);
	//	std::cout << "\n";
	//	/*highlightInText(std::cout, source, tokenOffsetInSource(token, source), token.source.size());*/
	//	highlightInText(std::cout, source, token.location.start, token.location.length);
	//	std::cout << "\n";
	//} 

	OstreamParserMessageReporter parserReporter(outputStream, source);
	Parser parser;
	const auto ast = parser.parse(tokens, functionNames, source, &parserReporter);
	if (ast.has_value()) {
		printExpr(ast->root, true);
		const auto outputRes = evaluateAst(ast->root, parameters, arguments);
		if (outputRes.isOk()) {
			put(" = %", outputRes.ok());
		} else {
			put("\nevaluation error: ", outputRes.err());
		}
	} else {
		put("parser error");
		return;
	}

	OstreamIrCompilerMessageReporter compilerReporter(outputStream, source);
	IrCompiler compiler;
	const auto irCode = compiler.compile(*ast, parameters, compilerReporter);
	if (irCode.has_value()) {
		put("original");
		printIrCode(std::cout, **irCode);
	} else {
		put("ir compiler error");
		return;
	}
	
	LocalValueNumbering valueNumbering;
	auto optimizedCode = valueNumbering.run(**irCode, parameters);
	const auto copy = optimizedCode;
	DeadCodeElimination deadCodeElimination;
	deadCodeElimination.run(copy, parameters, optimizedCode);
	//const auto optimizedCode = **irCode;

	put("optimized");
	printIrCode(std::cout, optimizedCode);

	IrVm vm;
	const auto irVmOutput = vm.execute(optimizedCode, arguments);
	if (irVmOutput.isOk()) {
		put("ir vm output: %", irVmOutput.ok());
	}

	CodeGenerator codeGenerator;
	auto machineCode = codeGenerator.compile(optimizedCode, parameters);

	outputToFile("test.txt", machineCode.code);

	/*const auto out = executeFunction(machineCode, arguments);
	put("out = %", out);*/
	/*bin.write(reinterpret_cast<const char*>(buffer), machineCode.size());*/
}

#include "test/simdFunctionsTest.hpp"

// https://stackoverflow.com/questions/4911993/how-to-generate-and-run-native-code-dynamically
int main(void) {
	test();
	//runFuzzTests();
	//testFloatingPointIdentites();
	//runTests();
	//testSimdFunctions();
}