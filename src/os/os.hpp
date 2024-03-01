#pragma once

#include "../utils/ints.hpp"

// Can be nullptr.
void* allocateExecutableMemory(i64 size);
void freeExecutableMemory(void* memory);