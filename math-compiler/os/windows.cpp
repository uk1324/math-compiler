#ifdef _WIN32

#include "../utils/ints.hpp"
#include "../utils/asserts.hpp"
#include <windows.h>

void* allocateExecutableMemory(i64 size) {
	return reinterpret_cast<u8*>(VirtualAllocEx(
		GetCurrentProcess(),
		nullptr, // No desired starting address
		size, // Rounds to page size
		MEM_COMMIT,
		PAGE_EXECUTE_READWRITE
	));
}

void freeExecutableMemory(void* memory) {
	const auto ret = VirtualFreeEx(GetCurrentProcess(), memory, 0, MEM_RELEASE);
	ASSERT(ret);
}

#endif