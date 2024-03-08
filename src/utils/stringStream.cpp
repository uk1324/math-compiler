#include "stringStream.hpp"

using namespace Calc;

StringStream::StringStream() :
	std::ostream(&buffer) {}

StringStream::int_type StringStream::StringStreamBuf::overflow(int_type c) {
	buffer += static_cast<char>(c);
	// Not sure what should be returned here. https://en.cppreference.com/w/cpp/io/basic_streambuf/overflow
	return 0;
}

std::string& StringStream::string() {
	return buffer.buffer;
}

const std::string& StringStream::string() const {
	return buffer.buffer;
}