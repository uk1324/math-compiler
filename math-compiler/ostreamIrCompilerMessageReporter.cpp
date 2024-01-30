#include "ostreamIrCompilerMessageReporter.hpp"
#include "utils/overloaded.hpp"
#include "utils/pritningUtils.hpp"
#include "utils/put.hpp"

OstreamIrCompilerMessageReporter::OstreamIrCompilerMessageReporter(std::ostream& output, std::string_view source) 
	: output(output)
	, source(source) {}

void OstreamIrCompilerMessageReporter::onError(const IrCompilerError& error) {
	std::visit(overloaded{
		[&](const UndefinedVariableIrCompilerError& e) {
			highlightInText(output, source, e.location.start, e.location.length);
			put(output, "variable '%' is not defined", e.undefinedVariableName);
		},
	}, error);
}
