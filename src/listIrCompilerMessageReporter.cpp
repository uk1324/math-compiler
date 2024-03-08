#include "listIrCompilerMessageReporter.hpp"

void ListIrCompilerMessageReporter::onError(const IrCompilerError& error) {
	errors.push_back(error);
}

void ListIrCompilerMessageReporter::reset() {
	errors.clear();
}
