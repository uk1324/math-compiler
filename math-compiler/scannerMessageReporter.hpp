#pragma once

#include "utils/ints.hpp"
#include <variant>
#include "sourceInfo.hpp"

struct IllegalCharScannerError {
	u8 character;
	i64 sourceOffset;

	bool operator==(const IllegalCharScannerError&) const = default;
};

struct InvalidIdentifierScannerError {
	std::string_view identifier;
	SourceLocation location;

	bool operator==(const InvalidIdentifierScannerError&) const = default;
};

using ScannerError = std::variant<
	IllegalCharScannerError,
	InvalidIdentifierScannerError
>;

// Maybe rename message handler.
struct ScannerMessageReporter {
	virtual void onError(const ScannerError& error) = 0;
};