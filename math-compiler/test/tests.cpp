#include "tests.hpp"
#include "../scanner.hpp"
#include "../parser.hpp"
#include "../irCompiler.hpp"
#include "../irVm.hpp"
#include "../codeGenerator.hpp"
#include "../evaluateAst.hpp"
#include "../executeFunction.hpp"
#include "testingParserMessageReporter.hpp"
#include "testingScannerMessageReporter.hpp"
#include "testingIrCompilerMessageReporter.hpp"
#include <sstream>
#include "../utils/pritningUtils.hpp"
#include "../utils/put.hpp"
#include "../utils/setDifference.hpp"

void runTests() {
	Scanner scanner;
	Parser parser;

	IrCompiler irCompiler;
	IrVm irVm;
	CodeGenerator codeGenerator;

	std::stringstream output;

	TestingScannerErrorReporter scannerReporter(output, std::string_view());
	TestingParserMessageReporter parserReporter(output, std::string_view());
	TestingIrCompilerMessageReporter irCompilerReporter(output, std::string_view());

	auto printFailed = [](std::string_view name) {
		put(TERMINAL_COLOR_RED "[FAILED] " TERMINAL_COLOR_RESET "%", name);
	};
	auto printPassed = [](std::string_view name) {
		put(TERMINAL_COLOR_GREEN "[PASSED] " TERMINAL_COLOR_RESET "%", name);
	};

	auto runTestExpected = [&](
		std::string_view name,
		std::string_view source,
		Real expectedOutput,
		std::span<const FunctionParameter> parameters = std::span<const FunctionParameter>(),
		std::span<const float> arguments = std::span<const float>()) {
			scannerReporter.reporter.source = source;
			parserReporter.reporter.source = source;
			//std::cout << "RUNNING TEST " << name " ";
			auto tokens = scanner.parse(source, &scannerReporter);
			if (scannerReporter.errorHappened) {
				printFailed(name);
				std::cout << "scanner error: \n" << output.str();
				output.clear();
				scannerReporter.reset();
				return;
			}
			auto ast = parser.parse(tokens, source, &parserReporter);
			if (!ast.has_value() && !parserReporter.errorHappened) {
				ASSERT_NOT_REACHED();
				return;
			}

			if (parserReporter.errorHappened) {
				printFailed(name);
				std::cout << "parser error: \n" << output.str();
				output.clear();
				parserReporter.reset();
				return;
			}

			bool evaluationError = false;

			{
				const auto output = evaluateAst(ast->root, parameters, arguments);
				if (output != expectedOutput) {
					printFailed(name);
					put("ast interpreter error:");
					if (output.ok()) {
						//put("expected '%' got '%'", expectedOutput, output.ok());
					}
					else {
						//put("message: %", output.err());
					}
					evaluationError = true;
				}
			}

			const auto irCode = irCompiler.compile(*ast, parameters, irCompilerReporter);
			if (irCode.has_value()) {
#ifdef DEBUG_PRINT_GENERATED_IR_CODE
				printIrCode(std::cout, **irCode);
#endif

				const auto output = irVm.execute(**irCode, arguments);
				if (!output.isOk()) {
					printFailed(name);
					put("ir vm runtime error");
				} else if (output.ok() != expectedOutput) {
					printFailed(name);
					std::cout << "ir vm error: ";
					std::cout << "expected '" << expectedOutput << "' got '" << output.ok() << "'\n";
					evaluationError = true;
				}
			} else {
				printFailed(name);
				std::cout << "ir compiler error: ";
				return;
			}

			{
				ASSERT(irCode.has_value());
				const auto code = codeGenerator.compile(**irCode);
				const auto output = executeFunction(codeGenerator, code, codeGenerator.data);
				if (output != expectedOutput) {
					printFailed(name);
					put("evaluation error: ");
					put("expected '%' got '%'", expectedOutput, output);
					evaluationError = true;
				}
			}

			if (evaluationError) {
				return;
			}

			printPassed(name);
	};

	// I choose to ignore additional errors that were reported, because things like adding parser synchronization might add additional errors.
	auto runTestExpectedErrors = [&](
		std::string_view name,
		std::string_view source,
		const std::vector<ScannerError>& expectedScannerErrors,
		const std::vector<ParserError>& expectedParserErrors,
		const std::vector<IrCompilerError>& expectedIrCompilerErrors,
		std::span<const FunctionParameter> parameters = std::span<const FunctionParameter>(),
		std::span<const float> arguments = std::span<const float>()) {

			scannerReporter.reporter.source = source;
			parserReporter.reporter.source = source;
			irCompilerReporter.reporter.source = source;


			auto tokens = scanner.parse(source, &scannerReporter);

			const auto scannerErrorsThatWereNotReported = setDifference(expectedScannerErrors, scannerReporter.errors);
			if (scannerErrorsThatWereNotReported.size() != 0) {
				printFailed(name);
				put("% scanner errors were not reported", scannerErrorsThatWereNotReported.size());
				put("output: \n%", output.str());
				output.str("");
				scannerReporter.reset();
				return;
			}

			auto ast = parser.parse(tokens, source, &parserReporter);
			if (!scannerReporter.errorHappened && !ast.has_value() && !parserReporter.errorHappened) {
				ASSERT_NOT_REACHED();
				return;
			}

			if (!ast.has_value()) {
				if (expectedIrCompilerErrors.size() == 0) {
					printPassed(name);
				}
				return;
			}

			irCompiler.compile(*ast, parameters, irCompilerReporter);
			const auto parserErrorsThatWereNotReported = setDifference(expectedParserErrors, parserReporter.errors);
			if (parserErrorsThatWereNotReported.size() != 0) {
				printFailed(name);
				put("% parser errors were not reported", parserErrorsThatWereNotReported.size());
				put("output: \n%", output.str());
				output.str("");
				parserReporter.reset();
				return;
			}

			const auto irCompilerErrorsThatWereNotReported = setDifference(expectedIrCompilerErrors, irCompilerReporter.errors);
			if (irCompilerErrorsThatWereNotReported.size() != 0) {
				printFailed(name);
				put("% ir compiler errors were not reported", parserErrorsThatWereNotReported.size());
				put("output: \n%", output.str());
				output.str("");
				irCompilerReporter.reset();
				return;
			}

			output.str("");
			printPassed(name);
	};

	runTestExpected("constant addition", "2 + 2", 4);
	runTestExpected("constant multiplication", "17 * 22", 374);
	runTestExpected("multiplication precedence", "2 + 3 * 4", 14);
	runTestExpected("parens", "(2 + 3) * 4", 20);
	runTestExpected("implicit multiplication", "4(2 + 3)", 20);

	runTestExpectedErrors(
		"illegal character",
		"?2 + 2",
		{ IllegalCharScannerError{.character = '?', .sourceOffset = 0 } },
		{},
		{}
	);

	runTestExpectedErrors(
		"expected token",
		"(2 + 2",
		{},
		{
			ExpectedTokenParserError{
				.expected = TokenType::RIGHT_PAREN,
				.found = Token(TokenType::END_OF_SOURCE, SourceLocation::fromStartLength(6, 0))
			}
		},
		{}
	);

	// This assumes that the last char is the last char in source of length 0.
	runTestExpectedErrors(
		"unexpected token",
		"(2 + ",
		{},
		{
			UnexpectedTokenParserError{
				.token = Token(TokenType::END_OF_SOURCE, SourceLocation::fromStartLength(5, 0))
			}
		},
		{}
	);

	runTestExpectedErrors(
		"undefined variable",
		"x",
		{},
		{},
		{
			UndefinedVariableIrCompilerError{
				.undefinedVariableName = "x",
				.location = SourceLocation(0, 1)
			}
		}
	);
}