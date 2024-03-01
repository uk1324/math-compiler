#include "ostreamScannerMessageReporter.hpp"
#include "utils/overloaded.hpp"
#include "utils/put.hpp"
#include "utils/pritningUtils.hpp"

OstreamScannerMessageReporter::OstreamScannerMessageReporter(std::ostream& output, std::string_view source)
	: output(output)
	, source(source) {}

void OstreamScannerMessageReporter::onError(const ScannerError& error) {
	std::visit(overloaded{
		[&](const IllegalCharScannerError& e) {
			put(output, "illegal character '%', code = '%'", static_cast<char>(e.character), static_cast<int>(e.character));
			highlightInText(output, source, e.sourceOffset, 1);
			put(output, "\n");
		},
		[&](const InvalidIdentifierScannerError& e) {
			put(output, "'%' is not a valid function or variable", e.identifier);
			highlightInText(output, source, e.location.start, e.location.length);
			put(output, "\n");
		}
	}, error);
}