#pragma once

#include <assert.h>

#define ASSERT(e) assert(e)
#define ASSERT_NOT_NEGATIVE(expr) ASSERT((expr) >= 0)
#define ASSERT_NOT_REACHED() ASSERT(0)