#pragma once

#include <variant>
#include <string_view>
#include "sourceInfo.hpp"

struct UndefinedVariableIrCompilerError {
	std::string_view undefinedVariableName;
	SourceLocation location;

	bool operator==(const UndefinedVariableIrCompilerError&) const = default;
};

struct InvalidNumberOfArgumentsIrCompilerError {
	std::string_view functionName;
	i64 argumentsFound;
	i64 argumentsExpected;
	SourceLocation location;

	bool operator==(const InvalidNumberOfArgumentsIrCompilerError&) const = default;
};

using IrCompilerError = std::variant<
	UndefinedVariableIrCompilerError,
	InvalidNumberOfArgumentsIrCompilerError
>;

struct IrCompilerMessageReporter {
	virtual void onError(const IrCompilerError& error) = 0;
};