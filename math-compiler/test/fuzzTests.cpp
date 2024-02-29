#include "fuzzTests.hpp"

#include "tests.hpp"
#include "randomInputGenerator.hpp"
#include "../scanner.hpp"
#include "../simdFunctions.hpp"
#include "../parser.hpp"
#include "../irCompiler.hpp"
#include "../codeGenerator.hpp"
#include "../evaluateAst.hpp"
#include "../executeFunction.hpp"
#include "../valueNumbering.hpp"
#include "../deadCodeElimination.hpp"
#include "../ostreamIrCompilerMessageReporter.hpp"
#include "../ostreamParserMessageReporter.hpp"
#include "../ostreamScannerMessageReporter.hpp"
#include "../runtimeUtils.hpp"
#include "../floatingPoint.hpp"
#include "../utils/put.hpp"
#include "../utils/format.hpp"
#include "../utils/stringStream.hpp"
#include "../utils/pritningUtils.hpp"
#include "../utils/fileIo.hpp"

struct FuzzTester {
	FuzzTester();

	Scanner scanner;
	Parser parser;

	IrCompiler irCompiler;
	CodeGenerator codeGenerator;
	LocalValueNumbering valueNumbering;
	DeadCodeElimination deadCodeElimination;

	StringStream output;
	OstreamScannerMessageReporter scannerReporter;
	OstreamParserMessageReporter parserReporter;
	OstreamIrCompilerMessageReporter irCompilerReporter;

	void initialize(std::string_view source);

	enum class RunResult {
		ERROR,
		SUCCESS
	};
	struct ValidInput {
		std::string_view source;
		std::span<const Variable> parameters;
		std::span<const float> arguments;
	};
	FunctionInfo functions[1]{
		{ .name = "exp", .arity = 1, .address = expSimd }
	};

	RunResult runValidInput(const ValidInput& in); 

	void runIncorrectSource(std::string_view source);
};

StringStream validSourceOutput;

FuzzTester::ValidInput generateValidInput(i32 maxParameterCount, i32 maxDepth) {
	validSourceOutput.string().clear();
	std::span<const Variable> parameters;
	return FuzzTester::ValidInput{
		//.source = generateValidSource(parameters, maxDepth)
	};
}

void runFuzzTests() {
	FuzzTester tester;
	StringStream out;

	RandomInputGenerator gen;
	const auto seed = 14;
	gen.rng.seed(seed);
	const auto parameterCount = 6;
	std::vector<Variable> paramters;
	out.clear();
	for (int i = 0; i < 5; i++) {
		const auto start = out.string().size();
		out << "x_" << i;
		const auto end = out.string().size();
		paramters.push_back(Variable{ .name = std::string_view(out.string().data() + start, end - start) });
	}
	std::vector<float> arguments;
	arguments.resize(paramters.size());

	std::uniform_int_distribution<u32> rng(0); 
	auto checkCorrectPrograms = [&] {
		for (int i = 0; i < 40000; i++) {
			for (i32 argumentIndex = 0; argumentIndex < paramters.size(); argumentIndex++) {
				const auto value = std::bit_cast<float>(rng(gen.rng));
				arguments[argumentIndex] = value;
			}

			const auto source = gen.generate(paramters, tester.functions);
			std::cout << source << '\n';
			const auto result = tester.runValidInput(FuzzTester::ValidInput{
				.source = source,
				.parameters = paramters,
				.arguments = arguments
			});
			if (result == FuzzTester::RunResult::ERROR) {
				put(TERMINAL_COLOR_RED "[FAILED] " TERMINAL_COLOR_RESET);
				std::cout << tester.output.string();
				return;
			}
			else {
				put(TERMINAL_COLOR_GREEN "[SUCCESS] " TERMINAL_COLOR_RESET);
			}
		}
	};

	auto checkIncorrectPrograms = [&] {
		std::string source;
		for (int i = 0; i < 400000; i++) {
			source = gen.generate(paramters, tester.functions);

			const i32 maxCharsToModify = 6;
			const auto charsToModify = gen.randomFrom0To(std::min(i32(source.length()), maxCharsToModify));
			for (i32 j = 0; j < charsToModify; j++) {
				const auto sourceIndex = gen.randomFrom0To(i32(source.length()));
				const auto randomCharIndex = gen.randomFrom0To(i32(std::size(TOKEN_LEGAL_CHARACTERS) - 1)); // -1 because of the null byte.
				const auto c = TOKEN_LEGAL_CHARACTERS[randomCharIndex];
				source[sourceIndex] = c;
			}

			std::cout << source << '\n';
			tester.runIncorrectSource(source);
		}
	};

	checkIncorrectPrograms();
	//checkCorrectPrograms();
}

FuzzTester::FuzzTester()
	: scannerReporter(output, std::string_view())
	, parserReporter(output, std::string_view())
	, irCompilerReporter(output, std::string_view()) {}

void FuzzTester::initialize(std::string_view source) {
	scannerReporter.source = source;
	irCompilerReporter.source = source;
	output.string().clear();
}

FuzzTester::RunResult FuzzTester::runValidInput(const ValidInput& in) {
	initialize(in.source);

	for (i32 i = 0; i < in.parameters.size(); i++) {
		put("% = %", in.parameters[i].name, in.arguments[i]);
	}

	const auto& tokens = scanner.parse(in.source, functions, in.parameters, scannerReporter);
	auto ast = parser.parse(tokens, in.source, parserReporter);
	if (!ast.has_value()) {
		return RunResult::ERROR;
	}

	const auto evaluateAstOutput = evaluateAst(ast->root, in.parameters, functions, in.arguments);

	const auto optIrCode = irCompiler.compile(*ast, in.parameters, functions, irCompilerReporter);

	const auto evaluateAstError = evaluateAstOutput.isErr();
	const auto irCompilerError = !optIrCode.has_value();
	if (evaluateAstError != irCompilerError) {
		put("evaluation error mismatch");
		return RunResult::ERROR;
	}
	if (evaluateAstError && irCompilerError) {
		return RunResult::SUCCESS;
	}

	auto irCode = &*optIrCode;

	auto optmizedIrCode = valueNumbering.run(*irCode, in.parameters);
	irCode = &optmizedIrCode;
	const auto copy = optmizedIrCode;
	deadCodeElimination.run(copy, in.parameters, optmizedIrCode);

	const auto machineCode = codeGenerator.compile(*irCode, functions, in.parameters);
	//outputToFile("test.txt", machineCode.code);
	const auto machineCodeOutput = executeFunction(machineCode, in.arguments);

	const float expected = evaluateAstOutput.ok();
	const float found = machineCodeOutput;

	if (f32BitwiseEquals(expected, found) || (isnan(expected) && isnan(found))) {
		return RunResult::SUCCESS;
	}

	put(output, "expected '%' found '%'", expected, found);
	put("input =");
	for (i32 i = 0; i < in.parameters.size(); i++) {
		put("% = %", in.parameters[i].name, in.arguments[i]);
	}
	for (i32 i = 0; i < in.parameters.size(); i++) {
		std::cout << "std::bit_cast<float>(0x" << std::hex << std::bit_cast<u32>(in.arguments[i]) << "), " << std::dec;
	}
	std::cout << '\n';
	return RunResult::ERROR;
}

void FuzzTester::runIncorrectSource(std::string_view source) {
	const auto& tokens = scanner.parse(source, {}, {}, scannerReporter);
	auto ast = parser.parse(tokens, source, parserReporter);
}
