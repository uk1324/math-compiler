#pragma once

#include <assert.h>

//[[noreturn]] void assertImplementation();

#define ASSERT(e) assert(e)
#define ASSERT_NOT_NEGATIVE(expr) ASSERT((expr) >= 0)
#define ASSERT_NOT_REACHED() ASSERT(0)