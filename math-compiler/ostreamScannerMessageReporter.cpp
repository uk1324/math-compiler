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
			put(output, "illegal character '%'", static_cast<char>(e.character));
			highlightInText(output, source, e.sourceOffset, 1);
			put(output, "\n");
		}
	}, error);
}