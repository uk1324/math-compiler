#pragma once

#include "parserMessageReporter.hpp"
#include <vector>

struct ListParserMessageReporter : public ParserMessageReporter {
	void onError(const ParserError& error) override;

	void reset();

	std::vector<ParserError> errors;
};