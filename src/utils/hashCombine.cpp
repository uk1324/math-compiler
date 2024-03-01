#include "hashCombine.hpp"

// Taken from boost.
usize hashCombine(usize a, usize b) {
	return a + 0x9e3779b9 + (b << 6) + (b >> 2);
}
