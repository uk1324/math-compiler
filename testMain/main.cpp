#include "scanner.hpp"
#include "parser.hpp"
#include "utils/fileIo.hpp"
#include "printAst.hpp"
#include "evaluateAst.hpp"
#include "ostreamScannerMessageReporter.hpp"
#include "ostreamParserMessageReporter.hpp"
#include "ostreamIrCompilerMessageReporter.hpp"
#include "irCompiler.hpp"
#include "irVm.hpp"
#include "codeGenerator.hpp"
#include "valueNumbering.hpp"
#include "deadCodeElimination.hpp"
#include "executeFunction.hpp"
#include "runtimeUtils.hpp"
#include "simdFunctions.hpp"

// TODO: Replace normal floating point equals with bitwise equals. !!!!!
void test() {

	std::string_view source = "17 * 22";
	Variable parameters[]{ { "x_0" }, { "x_1" }, { "x_2" }, { "x_3" }, { "x_4" } };
	float arguments[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

	std::ostream& outputStream = std::cerr;

	const FunctionInfo functionArray[] = {
		{.name = "exp", .arity = 1, .address = expSimd, }
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
		}
		else {
			put("\nevaluation error: %", outputRes.err());
		}
	}
	else {
		put("parser error");
		return;
	}

	OstreamIrCompilerMessageReporter compilerReporter(outputStream, source);
	IrCompiler compiler;
	const auto irCode = compiler.compile(*ast, parameters, functions, compilerReporter);
	if (irCode.has_value()) {
		put("original");
		printIrCode(std::cout, *irCode);
	}
	else {
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

	outputToFile("test.txt", machineCode.code);

	const auto out = executeFunction(machineCode, arguments);
	put("out = %", out);
}

int main(void) {
	test();
}