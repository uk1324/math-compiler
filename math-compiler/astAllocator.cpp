#include "astAllocator.hpp"
#include "utils/asserts.hpp"
#include <memory>

AstAllocator::AstAllocator()
	: current(nullptr)
	, first(nullptr) {

}

void* AstAllocator::allocate(i64 size, i64 alignment) {
	return malloc(size);
	//if (first == nullptr) {
	//	first = reinterpret_cast<Block*>(operator new(sizeof(Block)));
	//	current = first;
	//}
	//if (current == nullptr) {
	//	ASSERT_NOT_REACHED();
	//	current = first;
	//}
	//void* allocated = current->data + current->offset;
	//current->offset += size;
	////std::align(current->data + BLOCK_DATA_SIZE, size, current->data, alignment);
	//return allocated;
}
