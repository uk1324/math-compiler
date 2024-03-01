#include "testingParserMessageReporter.hpp"

TestingParserMessageReporter::TestingParserMessageReporter(std::ostream& ostream, std::string_view source) 
	: reporter(ostream, source) {
	reset();
}

void TestingParserMessageReporter::onError(const ParserError& error) {
	reporter.onError(error);
	errors.push_back(error);
	errorHappened = true;
}

void TestingParserMessageReporter::reset() {
	errorHappened = false;
	errors.clear();
}
