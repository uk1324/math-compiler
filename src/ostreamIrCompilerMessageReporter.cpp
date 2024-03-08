#include "ostreamIrCompilerMessageReporter.hpp"
#include "errorMessage.hpp"

OstreamIrCompilerMessageReporter::OstreamIrCompilerMessageReporter(std::ostream& output, std::string_view source) 
	: output(output)
	, source(source) {}

void OstreamIrCompilerMessageReporter::onError(const IrCompilerError& error) {
	outputIrCompilerErrorMessage(output, error, source, true);
}
