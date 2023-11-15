#pragma once

#include <string_view>
#include "ints.hpp"

#pragma once

namespace TerminalColors
{
	static const char* RED = "\x1B[31m";
	static const char* GREEN = "\x1B[32m";
	static const char* YELLOW = "\x1B[33m";
	static const char* BLUE = "\x1B[34m";
	static const char* MAGENTA = "\x1B[35m";
	static const char* CYAN = "\x1B[36m";
	static const char* WHITE = "\x1B[37m";
	static const char* RESET = "\x1B[0m";
}

void highlightInText(std::ostream& output, std::string_view text, i64 textToHighlightStart, i64 textToHighlightLength);