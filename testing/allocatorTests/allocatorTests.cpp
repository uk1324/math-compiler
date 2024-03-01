#include "scanner.hpp"
#include "parser.hpp"
#include "ostreamScannerMessageReporter.hpp"
#include "ostreamParserMessageReporter.hpp"
#include "ostreamIrCompilerMessageReporter.hpp"
#include "irCompiler.hpp"
#include "codeGenerator.hpp"
#include "simdFunctions.hpp"
#include <iostream>

void allocatorTest() {
	std::string_view source = "17 * 22";
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
		{.name = "exp", .arity = 1, .address = expSimd, }
	};

	for (i64 i = 0; i < 1000000; i++) {
		const auto& tokens = scanner.parse(source, functions, parameters, scannerReporter);
		const auto ast = parser.parse(tokens, source, parserReporter);
		/*const auto irCode = compiler.compile(*ast, parameters, functions, compilerReporter);
		const auto& machineCode = codeGenerator.compile(irCode, functions, parameters);*/
	}
}

int main() {
	allocatorTest();
}