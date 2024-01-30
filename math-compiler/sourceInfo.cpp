#include "sourceInfo.hpp"
#include "utils/asserts.hpp"

i64 SourceLocation::end() const {
	return start + length;
}

SourceLocation SourceLocation::fromStartEnd(i64 start, i64 end) {
	ASSERT_NOT_NEGATIVE(start);
	ASSERT_NOT_NEGATIVE(end);
	ASSERT(start <= end); // start = end can happen for example for END_OF_SOURCE tokens.
	return SourceLocation{
		.start = start,
		.length = end - start
	};
}

SourceLocation SourceLocation::fromStartLength(i64 start, i64 length) {
	ASSERT_NOT_NEGATIVE(start);
	ASSERT_NOT_NEGATIVE(length);
	return SourceLocation{
		.start = start,
		.length = length
	};
}
