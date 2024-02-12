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
#include "os/os.hpp"
#include "test/fuzzTests.hpp"
#include "test/tests.hpp"
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
	/*std::string_view source = "(9( (  (9)x_3 x_2 ( 45)x_3 *602x_1 ) )(    (  0.( 54.)( 6808.2)(  95591 )+(  71993)(0  )x_0 x_1 x_0   ) )  *  00(   (  821.87x_4 x_1 x_3 x_4  * 74219.x_1 ( 0798)( 13  )  ))(  ( x_2 x_1   *( 95.883)x_4  )  )x_1 x_0  )";*/
	// TO SEE THE BUG CHANGE THE CODE FOR INITIALIZE_COMMUTATIVE_BINARY_OP to make it not reorder values.
	std::string_view source = " (  26484x_4 (  9323.( 8)x_0 x_2 /x_0 ( 61860.2  )(  254. )x_0    )( ( 0 )( 41929 )( 825.)/ x_1 (656 )x_4 x_2   )x_1  /  x_2 (    (  7 )x_4 (  40)x_4 x_4 * (377)x_3  )(   (x_0 x_1  +(  22083.  )( 28.4  )(  60 )(705)) )x_0 )";
	/*std::string_view source = "1 + 2 * 3";*/
	/*std::string_view source = "xyz + 4(x + y)z";*/
	//std::string_view source = "x / y + -x";
	//std::string_view source = "2 + 2";
	//std::string_view source = "xy";
	//std::string_view source = "x + 1";
	//FunctionParameter parameters[] { { "x" }, { "y" }, { "z" } };
	FunctionParameter parameters[] { { "x_0" }, { "x_1" }, { "x_2" }, { "x_3" }, { "x_4" } };
	float arguments[] = { std::bit_cast<float>(0xf8e665f4), std::bit_cast<float>(0xff954a4a), std::bit_cast<float>(0x8d8a0542), std::bit_cast<float>(0x9b902791), std::bit_cast<float>(0x978140ff), };
// 
	//std::string_view source = "(x + y) + (x + y)";
	/*std::string_view source = "0.5772156649";*/
	//std::vector<Func
	//std::string_view source = "x + y";

	std::ostream& outputStream = std::cerr;

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
	const auto ast = parser.parse(tokens, source, &parserReporter);
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
	for (const auto& byte : machineCode.data) {
		std::cout << int(byte) << ' ';
	}

	const auto out = executeFunction(machineCode, arguments);
	put("out = %", out);
	/*bin.write(reinterpret_cast<const char*>(buffer), machineCode.size());*/
}

#include "test/simdFunctionsTest.hpp"

// https://stackoverflow.com/questions/4911993/how-to-generate-and-run-native-code-dynamically
int main(void) {
	test();
	//runFuzzTests();
	//runTests();
	// TODO: Write code that check when 2 functions return the same value (floating point comparasion and bitwise comparasion). Inputs could be arrays to make the generation of inputs easier. For inputs could use exacly representalbe numbers and not exacly representable numbers. Also iterate over all rounding modes.
	//testSimdFunctions();
}