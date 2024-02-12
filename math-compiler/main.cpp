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

void test() {
	std::string_view source = "  (( 720(6.755 )/ ( 6  )   )((  x_1 (0446 )  +  x_3 ( 83.859  )x_3 x_3 ))(   ( 54380x_0 ( 4 )/  087.(  6012.  )  ))( (  3018+ (6.  )(093.)) )(   ( (  33897.9  )x_2 x_0 x_0 +  (  8.81 )x_0 x_1 ( 331.  )))  * x_0 ( (99281-  6546 ))x_2 (  x_2 x_0   -3.21x_0   )x_4  )";
	/*std::string_view source = "1 + 2 * 3";*/
	/*std::string_view source = "xyz + 4(x + y)z";*/
	//std::string_view source = "x / y + -x";
	//std::string_view source = "2 + 2";
	//std::string_view source = "xy";
	//std::string_view source = "x + 1";
	//FunctionParameter parameters[] { { "x" }, { "y" }, { "z" } };
	FunctionParameter parameters[] { { "x_0" }, { "x_1" }, { "x_2" }, { "x_3" }, { "x_4" } };
	float arguments[] = { -2.48761e-28, -2.26242e-16, 5.49618e-33, -1.06439e-11, 1.53095e-08 };
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

	put("optimized");
	printIrCode(std::cout, optimizedCode);

	/*IrVm vm;
	vm.execute(**irCode, arguments);*/

	CodeGenerator codeGenerator;
	auto machineCode = codeGenerator.compile(optimizedCode, parameters);

	outputToFile("test.txt", machineCode.code);

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