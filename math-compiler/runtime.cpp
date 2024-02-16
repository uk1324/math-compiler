#include "runtime.hpp"

Runtime::Runtime(ScannerMessageReporter& scannerReporter, ParserMessageReporter& parserReporter, IrCompilerMessageReporter& irCompilerReporter)
    : scannerReporter(scannerReporter)
    , parserReporter(parserReporter)
    , irCompilerReporter(irCompilerReporter) {}

Runtime::LoopFunction Runtime::compileFunction(
    std::string_view name, 
    std::string_view source, 
    std::span<const FunctionParameter> parameters) {

    scanner.parse(source, functions, parameters, scannerReporter);

    return nullptr;
}
