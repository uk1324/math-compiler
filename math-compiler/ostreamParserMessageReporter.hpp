#pragma once

#include "parserMessageReporter.hpp"

struct OstreamParserMessageReporoter : public ParserMessageReporter {
	OstreamParserMessageReporoter(std::ostream& output, std::string_view source);
	void onError(const ParserError& error) override;

	std::ostream& output;
	std::string_view source;
};