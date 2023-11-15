#pragma once

#include "../ostreamScannerMessageReporter.hpp"
#include <vector>

struct TestingScannerErrorReporter : public ScannerMessageReporter {
	TestingScannerErrorReporter(std::ostream& output, std::string_view source);

	void onError(const ScannerError& error) override;

	void reset();

	bool errorHappened = false;
	std::vector<ScannerError> errors;

	OstreamScannerMessageReporter reporter;
};