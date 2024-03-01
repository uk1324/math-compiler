#pragma once

#include "scannerMessageReporter.hpp"
#include <string_view>

struct OstreamScannerMessageReporter final : public ScannerMessageReporter {
	OstreamScannerMessageReporter(std::ostream& output, std::string_view source);

	void onError(const ScannerError& error) override;

	std::ostream& output;
	std::string_view source;
};