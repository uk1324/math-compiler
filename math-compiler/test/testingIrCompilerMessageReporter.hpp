#pragma once

#include "../ostreamIrCompilerMessageReporter.hpp"
#include <vector>

struct TestingIrCompilerMessageReporter : public IrCompilerMessageReporter {
	TestingIrCompilerMessageReporter(std::ostream& ostream, std::string_view source);

	void onError(const IrCompilerError& error) override;

	void reset();

	bool errorHappened;
	std::vector<IrCompilerError> errors;

	OstreamIrCompilerMessageReporter reporter;
};