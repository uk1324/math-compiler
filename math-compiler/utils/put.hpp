#pragma once

#include <iostream>
#include "asserts.hpp"

// TODO: Currently there is not way to escape space so this isn't possible put("%%", a, b);

void putnn(std::ostream& os, const char* format);

template<typename T, typename ...Args>
void putnn(std::ostream& os, const char* format, const T& arg, const Args&... args) {
	int current = 0;
	while (format[current] != '\0') {
		if (format[current] == '%') {
			current++;
			// This is safe because the next one is either a valid character or the string end '\0'.
			if (format[current] == '%') {
				current++;
			}
			else {
				os << arg;
				putnn(os, format + current, args...);
				return;
			}
		}
		os << format[current];
		current++;
	}
	// A call with a correct format should always finish at the overload with no Args. 
	ASSERT_NOT_REACHED();
}

// nn - no newline

template<typename ...Args>
void putnn(std::ostream& os, const char* format, const Args&... args) {
	putNnTo(os, format, args...);
}

template<typename ...Args>
void put(std::ostream& os, const char* format, const Args&... args) {
	putnn(os, format, args...);
	os << '\n';
}

template<typename ...Args>
void put(const char* format, const Args&... args) {
	put(std::cout, format, args...);
}

template<typename ...Args>
void putnn(const char* format, const Args&... args) {
	putnn(std::cout, format, args...);
}