#include "fileIo.hpp"
#include <fstream>

void outputToFile(const char* filename, const std::vector<u8>& buffer) {
	std::ofstream bin(filename, std::ios::out | std::ios::binary);
	bin.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}
