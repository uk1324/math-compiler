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
#include "executeFunction.hpp"
#include "os/os.hpp"
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
	//std::string_view source = "2 + 2";
	/*std::string_view source = "1 + 2 * 3";*/
	/*std::string_view source = "xyz + 4(x + y)z";*/
	std::string_view source = "x / y + -x";
	//std::string_view source = "2 + 2";
	//std::string_view source = "xy";
	//std::string_view source = "x + 1";
	FunctionParameter parameters[] { { "x" }, { "y" }, { "z" } };
	float arguments[] = { 11.0f, 2.0f, 4.0f };
// 
	//std::string_view source = "(x + y) + (x + y)";
	/*std::string_view source = "0.5772156649";*/
	//std::vector<Func
	//std::string_view source = "x + y";

	std::ostream& outputStream = std::cerr;

	OstreamScannerMessageReporter scannerReporter(outputStream, source);
	Scanner scanner;
	const auto tokens = scanner.parse(source, &scannerReporter);
	for (auto& token : tokens) {
		debugOutputToken(token, source);
		std::cout << "\n";
		/*highlightInText(std::cout, source, tokenOffsetInSource(token, source), token.source.size());*/
		highlightInText(std::cout, source, token.location.start, token.location.length);
		std::cout << "\n";
	} 

	OstreamParserMessageReporoter parserReporter(outputStream, source);
	Parser parser;
	const auto ast = parser.parse(tokens, source, &parserReporter);
	if (ast.has_value()) {
		printExpr(ast->root, true);
		const auto outputRes = evaluateAst(ast->root, parameters, arguments);
		if (outputRes.ok()) {
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

// https://stackoverflow.com/questions/4911993/how-to-generate-and-run-native-code-dynamically
int main(void) {
	test();
	//runTests();
}