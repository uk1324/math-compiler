#include "ostreamParserMessageReporter.hpp"
#include "utils/overloaded.hpp"
#include "utils/pritningUtils.hpp"
#include "sourceInfo.hpp"

OstreamParserMessageReporoter::OstreamParserMessageReporoter(std::ostream& output, std::string_view source)
	: source(source)
	, output(output) {}

void OstreamParserMessageReporoter::onError(const ParserError& error) {
	std::visit(overloaded{
		[&](const UnexpectedTokenParserError& e) {
			const auto location = tokenSourceLocation(source, e.token);
			highlightInText(output, source, location.start, location.length);
		}
	}, error);
}
