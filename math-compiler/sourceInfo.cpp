#include "sourceInfo.hpp"

//i64 getLineFromOffset(std::string_view source, i64 sourceOffset) {
//	i64 line = 0;
//	for (i64 i = 0;)
//}

SourceLocation tokenSourceLocation(std::string_view source, const Token& token) {
	return SourceLocation{
		.start = token.source.data() - source.data(),
		.length = static_cast<i64>(token.source.size())
	};
}
