#pragma once

#include <sstream>

// The issue with std::stringstream is that it creates a copy of the string each time you access it. This class allows the user to directly access the internal std::string.
struct StringStream : std::ostream {
	struct StringStreamBuf : public std::stringbuf {
		int_type overflow(int_type c) override;
		std::string buffer;
	};
	StringStream();

	StringStreamBuf buffer;

	std::string& string();
	const std::string& string() const;
};