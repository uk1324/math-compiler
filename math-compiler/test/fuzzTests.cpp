#include "fuzzTests.hpp"

#include "tests.hpp"
#include "randomInputGenerator.hpp"
#include "../scanner.hpp"
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
#include "../utils/put.hpp"
#include "../utils/format.hpp"
#include "../utils/stringStream.hpp"

struct FuzzTester {
	FuzzTester();

	Scanner scanner;
	Parser parser;

	IrCompiler irCompiler;
	CodeGenerator codeGenerator;
	LocalValueNumbering valueNumbering;
	DeadCodeElimination deadCodeElimination;

	std::stringstream output;
	OstreamScannerMessageReporter scannerReporter;
	OstreamParserMessageReporter parserReporter;
	OstreamIrCompilerMessageReporter irCompilerReporter;

	void initialize(std::string_view source);

	enum class RunCorrectResult {
		ERROR,
		SUCCESS
	};
	struct ValidInput {
		std::string_view source;
		std::span<const FunctionParameter> parameters;
		std::span<const float> arguments;
	};

	RunCorrectResult runValidInput(const ValidInput& in);
	void runIncorrectSource(std::string_view source);
};

StringStream validSourceOutput;

FuzzTester::ValidInput generateValidInput(i32 maxParameterCount, i32 maxDepth) {
	validSourceOutput.string().clear();
	std::span<const FunctionParameter> parameters;
	return FuzzTester::ValidInput{
		//.source = generateValidSource(parameters, maxDepth)
	};
}

void runFuzzTests() {
	FuzzTester tester;
	StringStream out;

	RandomInputGenerator gen;
	const auto parameterCount = 5;
	std::vector<FunctionParameter> paramters;
	out.clear();
	for (int i = 0; i < 5; i++) {
		const auto start = out.string().size();
		out << "x_" << i;
		const auto end = out.string().size();
		paramters.push_back(FunctionParameter{ .name = std::string_view(out.string().data() + start, end - start) });
	}
	std::vector<float> arguments;
	arguments.resize(paramters.size());
	std::random_device engine;
	std::uniform_int_distribution<u32> rng(0);

	for (int i = 0; i < 20; i++) {
		for (i32 argumentIndex = 0; argumentIndex < paramters.size(); argumentIndex++) {
			const auto value = std::bit_cast<float>(rng(engine));
			arguments[argumentIndex] = value;
		}

		const auto source = gen.generate(paramters);
		std::cout << source << '\n';
		tester.runValidInput(FuzzTester::ValidInput{
			.source = source,
			.parameters = paramters,
		});
	}
}

FuzzTester::FuzzTester()
	: scannerReporter(output, std::string_view())
	, parserReporter(output, std::string_view())
	, irCompilerReporter(output, std::string_view()) {}

void FuzzTester::initialize(std::string_view source) {
	scannerReporter.source = source;
	irCompilerReporter.source = source;
}

FuzzTester::RunCorrectResult FuzzTester::runValidInput(const ValidInput& in) {
	initialize(in.source);

	auto tokens = scanner.parse(in.source, &scannerReporter);
	auto ast = parser.parse(tokens, in.source, &parserReporter);
	if (!ast.has_value()) {
		return RunCorrectResult::ERROR;
	}

	const auto evaluateAstOutput = evaluateAst(ast->root, in.parameters, in.arguments);

	const auto optIrCode = irCompiler.compile(*ast, in.parameters, irCompilerReporter);

	const auto evaluateAstError = evaluateAstOutput.isErr();
	const auto irCompilerError = !optIrCode.has_value();
	if (evaluateAstError != irCompilerError) {
		return RunCorrectResult::ERROR;
	}
	if (evaluateAstError) {
		return RunCorrectResult::SUCCESS;
	}

	auto irCode = *optIrCode;

	auto optmizedIrCode = valueNumbering.run(*irCode, in.parameters);
	irCode = &optmizedIrCode;
	const auto copy = optmizedIrCode;
	deadCodeElimination.run(copy, in.parameters, optmizedIrCode);

	const auto machineCode = codeGenerator.compile(*irCode, in.parameters);
	const auto machineCodeOutput = executeFunction(machineCode, in.arguments);
	if (machineCodeOutput != evaluateAstOutput) {
		return RunCorrectResult::ERROR;
	}
	return RunCorrectResult::SUCCESS;
}