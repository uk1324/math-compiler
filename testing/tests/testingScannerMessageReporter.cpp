#include "testingScannerMessageReporter.hpp"

TestingScannerErrorReporter::TestingScannerErrorReporter(std::ostream& output, std::string_view source) 
	: reporter(output, source) {
	reset();
}

void TestingScannerErrorReporter::onError(const ScannerError& error) {
	reporter.onError(error);
	errors.push_back(error);
	errorHappened = true;
}

void TestingScannerErrorReporter::reset() {
	errorHappened = false;
	errors.clear();
}
