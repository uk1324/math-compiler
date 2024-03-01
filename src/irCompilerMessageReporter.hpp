#pragma once

#include <variant>
#include <string_view>
#include "sourceInfo.hpp"

struct UndefinedVariableIrCompilerError {
	std::string_view undefinedVariableName;
	SourceLocation location;

	bool operator==(const UndefinedVariableIrCompilerError&) const = default;
};

using IrCompilerError = std::variant<UndefinedVariableIrCompilerError>;

struct IrCompilerMessageReporter {
	virtual void onError(const IrCompilerError& error) = 0;
};