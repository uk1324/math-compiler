#pragma once

#include "irCompilerMessageReporter.hpp"
#include "parserMessageReporter.hpp"
#include "scannerMessageReporter.hpp"
#include <ostream>

void outputScannerErrorMessage(std::ostream& out, const ScannerError& error, std::string_view source, bool printLocation);

void outputParserErrorMessage(std::ostream& out, const ParserError& error, std::string_view source, bool printLocation);

void outputIrCompilerErrorMessage(std::ostream& out, const IrCompilerError& error, std::string_view source, bool printLocation);