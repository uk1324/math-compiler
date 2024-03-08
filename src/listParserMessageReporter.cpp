#include "listParserMessageReporter.hpp"

void ListParserMessageReporter::onError(const ParserError& error) {
	errors.push_back(error);
}

void ListParserMessageReporter::reset() {
	errors.clear();
}
