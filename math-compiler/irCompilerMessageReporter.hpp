#pragma once

#include <variant>

struct TestError {

};

using IrCompilerError = std::variant<TestError>;

struct IrCompilerMessageReporter {
	virtual void onError(const IrCompilerError& error) = 0;
};