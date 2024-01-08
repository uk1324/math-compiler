#pragma once

#include <string_view>
#include <span>
#include "ints.hpp"
#include "../sourceInfo.hpp"

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

//void highlightInText(std::ostream& output, std::string_view text, std::span<const SourceLocation> upArrowHighlight, )

/*
TODO: Maybe implement error reporting like in GCC
(
	num * num;

<source>:4:18: error: expected ')' before ';' token
	4 |         num * num;
	  |                  ^
	  |                  )
<source>:3:5: note: to match this '('
	3 |     (
	  |     ^
*/

void highlightInText(std::ostream& output, std::string_view text, i64 textToHighlightStart, i64 textToHighlightLength);