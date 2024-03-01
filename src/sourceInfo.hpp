#pragma once

#include "utils/ints.hpp"
#include <string_view>

struct SourceLocation {
	i64 start;
	i64 length;
	i64 end() const;

	static SourceLocation fromStartEnd(i64 start, i64 end);
	static SourceLocation fromStartLength(i64 start, i64 length);

	bool operator==(const SourceLocation&) const = default;
};
