#pragma once

#include "utils/ints.hpp"
#include "token.hpp"
#include <string_view>

struct SourceLocation {
	i64 start;
	i64 length;
};

//i64 getLineFromOffset(std::string_view source, i64 sourceOffset);
SourceLocation tokenSourceLocation(std::string_view source, const Token& token);