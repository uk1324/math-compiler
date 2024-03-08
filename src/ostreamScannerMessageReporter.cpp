#include "ostreamScannerMessageReporter.hpp"
#include "errorMessage.hpp"

OstreamScannerMessageReporter::OstreamScannerMessageReporter(std::ostream& output, std::string_view source)
	: output(output)
	, source(source) {}

void OstreamScannerMessageReporter::onError(const ScannerError& error) {
	outputScannerErrorMessage(output, error, source, true);
}