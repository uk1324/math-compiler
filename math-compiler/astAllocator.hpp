#pragma once

#include "utils/ints.hpp"
#include <vector>
#include <span>

struct AstAllocator {
	template<typename T>
	struct List {
		void append(const T& value);
		std::span<const T> span() const;

		std::vector<T> d;
	};

	AstAllocator();

	void reset();

	std::vector<void*> allocations;

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
	allocations.push_back(memory);
	return reinterpret_cast<T*>(memory);
	//std::forward<Args...>(args...);
}

template<typename T>
void AstAllocator::List<T>::append(const T& value) {
	d.push_back(value);
}

template<typename T>
std::span<const T> AstAllocator::List<T>::span() const {
	return std::span<const T>(d);
}
