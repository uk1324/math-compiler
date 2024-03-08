#pragma once

#include "irCompilerMessageReporter.hpp"
#include <vector>

struct ListIrCompilerMessageReporter : public IrCompilerMessageReporter {
	void onError(const IrCompilerError& error) override;

	void reset();

	std::vector<IrCompilerError> errors;
};