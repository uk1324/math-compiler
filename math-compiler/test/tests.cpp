#include "tests.hpp"
#include "../scanner.hpp"
#include "../parser.hpp"
#include "../irCompiler.hpp"
#include "../irVm.hpp"
#include "../codeGenerator.hpp"
#include "../evaluateAst.hpp"
#include "../executeFunction.hpp"
#include "../valueNumbering.hpp"
#include "../deadCodeElimination.hpp"
#include "testingParserMessageReporter.hpp"
#include "testingScannerMessageReporter.hpp"
#include "testingIrCompilerMessageReporter.hpp"
#include <sstream>
#include "../utils/pritningUtils.hpp"
#include "../utils/put.hpp"
#include "../utils/setDifference.hpp"
#include "../utils/fileIo.hpp"

bool outputMachineCodeToFile = false;

std::string generateExpression(i64 depth, i64 maxDepth) {
	if (depth == maxDepth) {
		return format("x_%", depth);
	}
	return ::format("(x_% + %)", depth, generateExpression(depth + 1, maxDepth));
}
struct TestRunner {
	TestRunner();

	bool printIrGeneratedCode = false;
	bool printRemovedInstructionCount = false;

	Scanner scanner;
	Parser parser;

	IrCompiler irCompiler;
	IrVm irVm;
	CodeGenerator codeGenerator;
	LocalValueNumbering valueNumbering;
	DeadCodeElimination deadCodeElimination;

	std::stringstream output;

	TestingScannerErrorReporter scannerReporter;
	TestingParserMessageReporter parserReporter;
	TestingIrCompilerMessageReporter irCompilerReporter;

	void printPassed(std::string_view name);
	void printFailed(std::string_view name);

	void expectedHelper(
		std::string_view name,
		std::string_view source,
		Real expectedOutput,
		const std::vector<FunctionParameter>& parameters = std::vector<FunctionParameter>(),
		const std::vector<float>& arguments = std::vector<float>());

	void expected(
		std::string_view name,
		std::string_view source,
		Real expectedOutput,
		const std::vector<FunctionParameter>& parameters = std::vector<FunctionParameter>(),
		const std::vector<float>& arguments = std::vector<float>());

	void expectedErrorsHelper(
		std::string_view name,
		std::string_view source,
		const std::vector<ScannerError>& expectedScannerErrors,
		const std::vector<ParserError>& expectedParserErrors,
		const std::vector<IrCompilerError>& expectedIrCompilerErrors,
		std::span<const FunctionParameter> parameters = std::span<const FunctionParameter>(),
		std::span<const float> arguments = std::span<const float>());
	void expectedErrors(
		std::string_view name,
		std::string_view source,
		const std::vector<ScannerError>& expectedScannerErrors,
		const std::vector<ParserError>& expectedParserErrors,
		const std::vector<IrCompilerError>& expectedIrCompilerErrors,
		std::span<const FunctionParameter> parameters = std::span<const FunctionParameter>(),
		std::span<const float> arguments = std::span<const float>());

	void reset();
};


void runTests() {
	TestRunner t;

	// I choose to ignore additional errors that were reported, because things like adding parser synchronization might add additional errors.

	t.expected("constant addition", "2 + 2", 4);
	t.expected("constant multiplication", "17 * 22", 374);
	t.expected("multiplication precedence", "2 + 3 * 4", 14);
	t.expected("parens", "(2 + 3) * 4", 20);
	t.expected("implicit multiplication", "4(2 + 3)", 20);
	t.expected("variable loading", "x", 3.14f, { { "x" } }, { { 3.14f } });
	t.expected("addition", "x + y", 6.0f, { { "x" }, { "y" } }, { { 2.0f, 4.0f } });
	t.expected("subtraction", "x - y", -2.0f, { { "x" }, { "y" } }, { { 2.0f, 4.0f } });
	t.expected("multiplication", "x * y", 8.0f, { { "x" }, { "y" } }, { { 2.0f, 4.0f } });
	t.expected("division ", "x / y", 0.5, { { "x" }, { "y" } }, { { 2.0f, 4.0f } });
	t.expected("negation", "-x", -2.0f, { { "x" }, }, { { 2.0f } });
	t.expected("constant addition", "2 + 4", 6.0f);
	t.expected("constant subtraction", "2 - 4", -2.0f);
	t.expected("constant multiplication", "2 * 4", 8.0f);
	t.expected("constant division ", "2 / 4", 0.5);
	t.expected("constant negation", "-2", -2.0f);
	t.expected(
		"more variables", 
		"xyz + 4(x + y)z", 
		296, 
		{ { { "x" }, { "y" }, { "z" } } },
		{ { 11.0f, 2.0f, 4.0f } });
	{
		std::vector<std::string> variableNames;
		std::vector<FunctionParameter> parameters;
		std::vector<float> arguments;
		const auto variableCount = 64;
		for (int i = 0; i < variableCount; i++) {
			variableNames.push_back(format("x_%", i));
			arguments.push_back(1.0f);
		}
		for (int i = 0; i < variableCount; i++) {
			parameters.push_back({ variableNames[i] });
		}
		std::stringstream source;
		for (int i = 0; i < variableCount; i++) {
			putnn(source, "x_% + ", i);
		}
		for (int i = variableCount - 1; i >= 0; i--) {
			putnn(source, "x_%", i);
			if (i != 0) {
				putnn(source, " + ");
			}
		}

		t.expected("a lot of variables", source.str(), 128.0f, parameters, arguments);
	}

	// Value numbering
	t.expected("duplicate expression", "(a + b) + (a + b)", 6.0f, { { "a" }, { "b" } }, { { 1.0f }, { 2.0f } });
	t.expected("multiplication by 1", "x * 1", 5.0f, { { "x" } }, { { 5.0f } });
	t.expected("multiplication by 2", "x * 2", 10.0f, { { "x" } }, { { 5.0f } });
	t.expected("division by 1", "x / 1", 5.0f, { { "x" } }, { { 5.0f } });

	t.expectedErrors(
		"illegal character",
		"?2 + 2",
		{ IllegalCharScannerError{.character = '?', .sourceOffset = 0 } },
		{},
		{}
	);

	t.expectedErrors(
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
	t.expectedErrors(
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

	t.expectedErrors(
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

TestRunner::TestRunner()
	: scannerReporter(output, std::string_view())
	, parserReporter(output, std::string_view())
	, irCompilerReporter(output, std::string_view()) {}

void TestRunner::printPassed(std::string_view name) {
	put(TERMINAL_COLOR_GREEN "[PASSED] " TERMINAL_COLOR_RESET "%", name);
}

void TestRunner::printFailed(std::string_view name) {
	put(TERMINAL_COLOR_RED "[FAILED] " TERMINAL_COLOR_RESET "%", name);
}

void TestRunner::expectedHelper(std::string_view name, std::string_view source, Real expectedOutput, const std::vector<FunctionParameter>& parameters, const std::vector<float>& arguments) {
	scannerReporter.reporter.source = source;
	parserReporter.reporter.source = source;

	auto tokens = scanner.parse(source, &scannerReporter);
	if (scannerReporter.errorHappened) {
		printFailed(name);
		put("scanner error: %", output.str());
		return;
	}

	auto ast = parser.parse(tokens, source, &parserReporter);
	if (!ast.has_value() && !parserReporter.errorHappened) {
		ASSERT_NOT_REACHED();
		printFailed(name);
		return;
	}

	if (parserReporter.errorHappened) {
		printFailed(name);
		put("parser error: %", output.str());
		return;
	}

	{
		const auto output = evaluateAst(ast->root, parameters, arguments);

		if (output != expectedOutput) {
			printFailed(name);
			put("ast interpreter error:");
			if (output.isOk()) {
				put("expected '%' got '%'", expectedOutput, output.ok());
			}
			else {
				put("message: %", output.err());
			}
			return;
		}
	}

	const auto optIrCode = irCompiler.compile(*ast, parameters, irCompilerReporter);

	if (!optIrCode.has_value()) {
		printFailed(name);
		put("ir compiler error");
		return;
	}

	auto irCode = *optIrCode;

	if (printIrGeneratedCode) {
		put("original:");
		printIrCode(std::cout, *irCode);
	}

	auto optmizedIrCode = valueNumbering.run(*irCode, parameters);
	irCode = &optmizedIrCode;

	const auto copy = optmizedIrCode;
	deadCodeElimination.run(copy, parameters, optmizedIrCode);

	if (printIrGeneratedCode) {
		put("optimized:");
		printIrCode(std::cout, optmizedIrCode);
	}


	if (printRemovedInstructionCount) {
		put("removed instruction count %", i64((*optIrCode)->size()) - i64(irCode->size()));
	}

	{
		const auto output = irVm.execute(*irCode, arguments);
		//put("removed instructions: %", i64((*irCode)->size()) - i64(optimizedCode.size()));

		if (output.isErr()) {
			printFailed(name);
			put("ir vm runtime error: %", output.err());
			return;
		}

		if (output.ok() != expectedOutput) {
			printFailed(name);
			put("ir vm error: ");
			put("expected '%' got '%' ", expectedOutput, output.ok());
			return;
		}
	}

	{
		const auto machineCode = codeGenerator.compile(*irCode, parameters);
		if (outputMachineCodeToFile) {
			outputToFile("test.txt", machineCode.code);
		}

		const auto output = executeFunction(machineCode, arguments);
		if (output != expectedOutput) {
			printFailed(name);
			put("evaluation error: ");
			put("expected '%' got '%'", expectedOutput, output);
		}
	}

	printPassed(name);
}

void TestRunner::expected(std::string_view name, std::string_view source, Real expectedOutput, const std::vector<FunctionParameter>& parameters, const std::vector<float>& arguments) {

	expectedHelper(name, source, expectedOutput, parameters, arguments);
	reset();
}

void TestRunner::expectedErrorsHelper(std::string_view name, std::string_view source, const std::vector<ScannerError>& expectedScannerErrors, const std::vector<ParserError>& expectedParserErrors, const std::vector<IrCompilerError>& expectedIrCompilerErrors, std::span<const FunctionParameter> parameters, std::span<const float> arguments) {
	auto tokens = scanner.parse(source, &scannerReporter);

	const auto scannerErrorsThatWereNotReported = setDifference(expectedScannerErrors, scannerReporter.errors);
	if (scannerErrorsThatWereNotReported.size() != 0) {
		printFailed(name);
		put("% scanner errors were not reported", scannerErrorsThatWereNotReported.size());
		put("output: \n%", output.str());
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
		return;
	}

	const auto irCompilerErrorsThatWereNotReported = setDifference(expectedIrCompilerErrors, irCompilerReporter.errors);
	if (irCompilerErrorsThatWereNotReported.size() != 0) {
		printFailed(name);
		put("% ir compiler errors were not reported", parserErrorsThatWereNotReported.size());
		put("output: \n%", output.str());
		return;
	}

	printPassed(name);
}

void TestRunner::expectedErrors(std::string_view name, std::string_view source, const std::vector<ScannerError>& expectedScannerErrors, const std::vector<ParserError>& expectedParserErrors, const std::vector<IrCompilerError>& expectedIrCompilerErrors, std::span<const FunctionParameter> parameters, std::span<const float> arguments) {
	scannerReporter.reporter.source = source;
	parserReporter.reporter.source = source;
	irCompilerReporter.reporter.source = source;
	expectedErrorsHelper(name, source, expectedScannerErrors, expectedParserErrors, expectedIrCompilerErrors, parameters, arguments);
	reset();
}

void TestRunner::reset() {
	output.str("");
	// could add reset to the interface and just call it in (Parser|Scanner|Compiler)::initialize().
	scannerReporter.reset();
	parserReporter.reset();
	irCompilerReporter.reset();
}
