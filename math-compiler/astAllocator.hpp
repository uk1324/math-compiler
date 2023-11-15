#pragma once

#include "utils/ints.hpp"

struct AstAllocator {
	AstAllocator();

	static constexpr i64 BLOCK_DATA_SIZE = 4096;
	struct Block {
		u8 data[BLOCK_DATA_SIZE];
		i64 offset = 0;
		Block* next = nullptr;
	};
	Block* first;
	Block* current;

	void* allocate(i64 size, i64 alignment);
	template<typename T, typename ...Args>
	T* allocate(Args&&... args);
};

template<typename T, typename ...Args>
T* AstAllocator::allocate(Args&&... args) {
	void* memory = allocate(sizeof(T), alignof(T));
	new (memory) T(args...);
	return reinterpret_cast<T*>(memory);
	//std::forward<Args...>(args...);
}