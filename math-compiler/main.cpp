#include <iostream>
#include "scanner.hpp"
#include "parser.hpp"
#include "utils/asserts.hpp"
#include "printAst.hpp"
#include "evaluateAst.hpp"
#include "utils/pritningUtils.hpp"
#include "ostreamScannerMessageReporter.hpp"
#include "ostreamParserMessageReporter.hpp"
#include "../math-compiler/test/testingParserMessageReporter.hpp"
#include "../math-compiler/test/testingScannerMessageReporter.hpp"
#include "utils/put.hpp"
#include "irCompiler.hpp"
#include "irVm.hpp"
#include "codeGenerator.hpp"
#include <span>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <windows.h>

// Can be nullptr.
void* allocateExecutableMemory(i64 size) {
	return reinterpret_cast<u8*>(VirtualAllocEx(
		GetCurrentProcess(),
		nullptr, // No desired starting address
		size, // Rounds to page size
		MEM_COMMIT,
		PAGE_EXECUTE_READWRITE
	));
}

void freeExecutableMemory(void* memory) {
	const auto ret = VirtualFreeEx(GetCurrentProcess(), memory, 0, MEM_RELEASE);
	ASSERT(ret);
}

Real executeProgram(const CodeGenerator& codeGenerator, const std::vector<u8>& code, const std::vector<u8>& data) {
	auto codeBuffer = reinterpret_cast<u8*>(allocateExecutableMemory(code.size() + data.size()));
	if (codeBuffer == nullptr) {
		put("failed to allocate executable memory");
		exit(EXIT_FAILURE);
	}

	memcpy(codeBuffer, code.data(), code.size());
	const auto dataBuffer = codeBuffer + code.size();
	memcpy(dataBuffer, data.data(), data.size());
	codeGenerator.patchExecutableCodeRipAddresses(codeBuffer, data.data());

	using Function = float (*)(void);

	const auto function = reinterpret_cast<Function>(codeBuffer);
	return function();
}

i64 tokenOffsetInSource(const Token& token, std::string_view originalSource) {
	const auto offsetInOriginalSource = token.source.data() - originalSource.data();
	return offsetInOriginalSource;
}

void debugOutputToken(const Token& token, std::string_view originalSource) {
	const auto offsetInOriginalSource = tokenOffsetInSource(token, originalSource);
	std::cout << offsetInOriginalSource << " " << offsetInOriginalSource + token.source.size() << " = ";
	std::cout << tokenToStr(token);
	std::cout << " '" << token.source << "'";
}

template<typename T>
std::vector<T> setDifference(const std::vector<T>& as, const std::vector<T>& bs) {
	std::vector<T> difference;
	for (const auto& a : as) {
		bool aInBs = false;
		for (const auto& b : bs) {
			if (a == b) {
				aInBs = true;
				break;
			}
		}
		if (!aInBs) {
			difference.push_back(a);
		}
	}
	return difference;
}

void runTests() {
	Scanner scanner;
	Parser parser;

	IrCompiler irCompiler;
	IrVm irVm;
	CodeGenerator codeGenerator;

	std::stringstream output;

	TestingScannerErrorReporter scannerReporter(output, std::string_view());
	TestingParserMessageReporter parserReporter(output, std::string_view());

	auto printFailed = [](std::string_view name) {
		std::cout << TerminalColors::RED << "[FAILED] " << TerminalColors::RESET << name << '\n';
	};
	auto printPassed = [](std::string_view name) {
		std::cout << TerminalColors::GREEN << "[PASSED] " << TerminalColors::RESET << name << '\n';
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
		auto ast = parser.parse(tokens, &parserReporter);
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
				} else {
					//put("message: %", output.err());
				}
				evaluationError = true;
			}
		}

		const auto irCode = irCompiler.compile(*ast);
		if (irCode.has_value()) {
			#ifdef DEBUG_PRINT_GENERATED_IR_CODE
			printIrCode(std::cout, **irCode);
			#endif

			const auto output = irVm.execute(**irCode);
			if (!output.has_value()) {
				printFailed(name);
				put("ir vm runtime error");
			} else if (*output != expectedOutput) {
				printFailed(name);
				std::cout << "ir vm error: ";
				std::cout << "expected '" << expectedOutput << "' got '" << *output << "'\n";
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
			const auto output = executeProgram(codeGenerator, code, codeGenerator.data);
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
		const std::vector<ParserError>& expectedParserErrors) {

		scannerReporter.reporter.source = source;
		parserReporter.reporter.source = source;

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

		auto ast = parser.parse(tokens, &parserReporter);
		if (!scannerReporter.errorHappened && !ast.has_value() && !parserReporter.errorHappened) {
			ASSERT_NOT_REACHED();
			return;
		}

		setDifference(expectedParserErrors, parserReporter.errors);
		const auto parserErrorsThatWereNotReported = setDifference(expectedParserErrors, parserReporter.errors);
		if (parserErrorsThatWereNotReported.size() != 0) {
			printFailed(name);
			put("% parser errors were not reported", parserErrorsThatWereNotReported.size());
			put("output: \n%", output.str());
			output.str("");
			parserReporter.reset();
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
		{ IllegalCharScannerError{ .character = '?', .sourceOffset = 0 }},
		{}
	);

	{
		const std::string_view source = "(2 + 2";
		const auto errorText = source.substr(6, 0);
		runTestExpectedErrors(
			"expected token",
			source,
			{},
			{ 
				ExpectedTokenParserError{
					.expected = TokenType::RIGHT_PAREN,
					.found = Token(TokenType::END_OF_SOURCE, errorText)
				}
			}
		);
	}

	{
		const std::string_view source = "(2 + ";
		// This assumes that the last char is the last char in source of length 0.
		const auto errorText = source.substr(5, 0);
		runTestExpectedErrors(
			"unexpected token",
			source,
			{},
			{
				UnexpectedTokenParserError{
					.token = Token(TokenType::END_OF_SOURCE, errorText)
				}
			}
		);
	}
}

#include "bashPath.hpp"

void test() {
	/*std::string_view source = "2 + 2";*/
	/*std::string_view source = "1 + 2 * 3";*/
	std::string_view source = "xyz + 4(x + y)z";
	FunctionParameter parameters[] { { "x" }, { "y" }, { "z" } };
	float arguments[] = { 1.0f, 2.0f, 4.0f };
	//std::string_view source = "(2 + 3) * 4";
	/*std::string_view source = "0.5772156649";*/
	//std::vector<Func

	std::ostream& outputStream = std::cerr;

	OstreamScannerMessageReporter scannerReporter(outputStream, source);
	Scanner scanner;
	const auto tokens = scanner.parse(source, &scannerReporter);
	for (auto& token : tokens) {
		debugOutputToken(token, source);
		std::cout << "\n";
		highlightInText(std::cout, source, tokenOffsetInSource(token, source), token.source.size());
		std::cout << "\n";
	} 

	OstreamParserMessageReporoter parserReporter(outputStream, source);
	Parser parser;
	const auto ast = parser.parse(tokens, &parserReporter);
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

	IrCompiler compiler;
	const auto irCode = compiler.compile(*ast);
	if (irCode.has_value()) {
		printIrCode(std::cout, **irCode);
	}

	CodeGenerator codeGenerator;
	auto machineCode = codeGenerator.compile(**irCode);
	const auto out = executeProgram(codeGenerator, machineCode, codeGenerator.data);
	put("out = %", out);

	std::ofstream bin("test.txt", std::ios::out | std::ios::binary);
	bin.write(reinterpret_cast<const char*>(machineCode.data()), machineCode.size());
	/*bin.write(reinterpret_cast<const char*>(buffer), machineCode.size());*/
}

// https://stackoverflow.com/questions/4911993/how-to-generate-and-run-native-code-dynamically
int main(void) {
	//test();
	runTests();
}