#include "listScannerMessageReporter.hpp"

void ListScannerMessageReporter::onError(const ScannerError& error) {
	errors.push_back(error);
}

void ListScannerMessageReporter::reset() {
	errors.clear();
}
