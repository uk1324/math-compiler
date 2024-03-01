#pragma once

#include "ostreamParserMessageReporter.hpp"
#include <vector>

struct TestingParserMessageReporter : public ParserMessageReporter {
	TestingParserMessageReporter(std::ostream& ostream, std::string_view source);

	void onError(const ParserError& error) override;

	void reset();

	bool errorHappened;
	std::vector<ParserError> errors;

	OstreamParserMessageReporter reporter;
};