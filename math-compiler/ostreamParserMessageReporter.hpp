#pragma once

#include "parserMessageReporter.hpp"

struct OstreamParserMessageReporoter : public ParserMessageReporter {
	OstreamParserMessageReporoter(std::ostream& output, std::string_view source);
	void onError(const ParserError& error);

	std::ostream& output;
	std::string_view source;
};