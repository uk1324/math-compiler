#pragma once

#include <stdint.h>

template <typename A, typename B>
A roundUpToMultiple(A value, B alignment);

template<typename T>
T* roundAddressUpToMultiple(T* address, intptr_t alignment);

template<typename A, typename B>
A roundUpToMultiple(A value, B alignment) {
	const auto misalignment = value % alignment;
	return misalignment == 0
		? value
		: value + (alignment - misalignment);
}

template<typename T>
T* roundAddressUpToMultiple(T* address, intptr_t alignment) {
	return reinterpret_cast<T*>(roundUpToMultiple(intptr_t(address), alignment));
}
