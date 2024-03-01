#pragma once

#include "parserMessageReporter.hpp"

struct OstreamParserMessageReporter : public ParserMessageReporter {
	OstreamParserMessageReporter(std::ostream& output, std::string_view source);
	void onError(const ParserError& error) override;

	std::ostream& output;
	std::string_view source;
};