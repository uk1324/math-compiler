#include "testingIrCompilerMessageReporter.hpp"

TestingIrCompilerMessageReporter::TestingIrCompilerMessageReporter(std::ostream& ostream, std::string_view source) 
	: reporter(ostream, source) {
	reset();
}

void TestingIrCompilerMessageReporter::onError(const IrCompilerError& error) {
	reporter.onError(error);
	errors.push_back(error);
	errorHappened = true;
}

void TestingIrCompilerMessageReporter::reset() {
	errorHappened = false;
	errors.clear();
}
