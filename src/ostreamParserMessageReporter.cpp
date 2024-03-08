#include "ostreamParserMessageReporter.hpp"
#include "errorMessage.hpp"

OstreamParserMessageReporter::OstreamParserMessageReporter(std::ostream& output, std::string_view source)
	: source(source)
	, output(output) {}

void OstreamParserMessageReporter::onError(const ParserError& error) {
	outputParserErrorMessage(output, error, source, true);
}
