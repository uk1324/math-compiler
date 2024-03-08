#pragma once

#include "scannerMessageReporter.hpp"
#include <vector>

struct ListScannerMessageReporter : public ScannerMessageReporter {
	void onError(const ScannerError& error) override;

	void reset();

	std::vector<ScannerError> errors;
};