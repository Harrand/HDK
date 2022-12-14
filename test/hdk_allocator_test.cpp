#include "hdk/memory/allocators/adapter.hpp"
#include "hdk/memory/allocators/linear.hpp"
#include "hdk/memory/allocators/malloc.hpp"
#include "hdk/memory/allocators/null.hpp"
#include "hdk/memory/allocators/stack.hpp"
#include "hdk/debug.hpp"
#include <type_traits>
#include <vector>
#include <deque>
#include <numeric>

void null_allocator_tests()
{
	hdk::null_allocator na;
	hdk::memblk blk = na.allocate(1);
	hdk::assert(blk == hdk::nullblk, "null_allocator did not return a null allocation.");
	hdk::assert(na.owns(blk), "null_allocator thinks it doesn't own the block it created.");
	na.deallocate(blk);
}

void mallocator_test()
{
	hdk::mallocator ma;
	hdk::memblk blk = ma.allocate(1024);
	hdk::assert(blk != hdk::nullblk, "mallocator failed to alloc 1024 bytes.");
	hdk::assert(ma.owns(blk), "mallocator thinks it does not own the block it created.");

	std::vector<int, hdk::allocator_adapter<int, hdk::mallocator>> ints{1, 2, 3, 4};
	ints.clear();
	ints.push_back(5);

	std::deque<int, hdk::allocator_adapter<int, hdk::mallocator>> int_deque;
	int_deque.push_back(0);
	int_deque.push_front(1);
	int_deque.clear();
	int_deque.resize(10);
	std::iota(int_deque.begin(), int_deque.end(), 0);
}

void stack_allocator_tests()
{
	hdk::stack_allocator<64> s;
	auto blk2 = s.allocate(1);
	hdk::assert(s.owns(blk2) && !s.owns(hdk::nullblk), "stack_allocator says it doesn't own a block it allocated, or thinks it owns nullblk");
	s.deallocate(blk2);

	using LocalAllocator = hdk::stack_allocator<64>;
	std::vector<int, hdk::allocator_adapter<int, LocalAllocator>> ints2;
	ints2.resize(4);
	ints2.push_back(5);
	ints2.push_back(6);
}

void linear_allocator_tests()
{
	// First create some storage we can use.
	hdk::mallocator mallocator;
	hdk::memblk scratch = mallocator.allocate(256);
	{
		hdk::linear_allocator lalloc{scratch};
		hdk::memblk blk = lalloc.allocate(256);
		hdk::assert(lalloc.owns(blk), "linear_allocator allocated its entire contents, but says it doesn't own the resultant block");
		lalloc.deallocate(blk);
		hdk::assert((blk = lalloc.allocate(1)) != hdk::nullblk, "linear_allocator thinks it ran out of space but I deallocated it to empty, the next allocation returned the null block.");
		lalloc.deallocate(blk);
		hdk::assert(lalloc.allocate(257) == hdk::nullblk, "linear_allocator ran out of space, but didn't output the null block.");

		// The adapter linear allocator will happily overwrite any previous allocations.
		hdk::allocator_adapter<int, hdk::linear_allocator> int_lalloc{lalloc};
		std::vector<int, hdk::allocator_adapter<int, hdk::linear_allocator>> ints{int_lalloc};
		ints.reserve(4);
		ints.push_back(1);
		ints.push_back(2);

		// Check memory values
		hdk::assert(*(reinterpret_cast<int*>(scratch.ptr) + 0) == 1, "linear_allocator is allocating in a dodgy area in its arena.");
		hdk::assert(*(reinterpret_cast<int*>(scratch.ptr) + 1) == 2, "linear_allocator is allocating in a dodgy area in its arena.");

		ints.clear();
		ints.push_back(3);
		hdk::assert(*(reinterpret_cast<int*>(scratch.ptr) + 0) == 3, "linear_allocator is allocating in a dodgy area in its arena (post-dealloc)");
	}
	mallocator.deallocate(scratch);
}

int main()
{
	null_allocator_tests();
	mallocator_test();
	stack_allocator_tests();
	linear_allocator_tests();
}
