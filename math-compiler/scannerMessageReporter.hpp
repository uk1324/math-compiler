#pragma once

#include "utils/ints.hpp"
#include <variant>

struct IllegalCharScannerError {
	u8 character;
	i64 sourceOffset;

	bool operator==(const IllegalCharScannerError&) const = default;
};

using ScannerError = std::variant<IllegalCharScannerError>;

// Maybe rename message handler.
struct ScannerMessageReporter {
	virtual void onError(const ScannerError& error) = 0;
};