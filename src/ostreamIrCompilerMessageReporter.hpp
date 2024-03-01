#pragma once

#include "irCompilerMessageReporter.hpp"
#include <ostream>

struct OstreamIrCompilerMessageReporter final : public IrCompilerMessageReporter {
	OstreamIrCompilerMessageReporter(std::ostream& output, std::string_view source);

	void onError(const IrCompilerError& error) override;

	std::ostream& output;
	std::string_view source;
};