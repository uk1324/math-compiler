#pragma once

#include "put.hpp"
#include <sstream>

template<typename ...Args>
std::string format(const char* format, const Args&... args) {
	// @Performance: Maybe have a static threadlocal variable that returns a const reference. Or use my custon stringstream that allows moving the string out of the stream.
	std::stringstream stream;
	putnn(stream, format, args...);
	return stream.str();
}

//template<typename ...Args>
//std::string formatnn(const char* format, const Args&... args) {
//	return format(format, args);
//}