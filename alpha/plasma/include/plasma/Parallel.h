// Compatibility wrappers for parallel loops and concurrent vectors. MSVC uses its Concurrency Runtime; other compilers
// execute loops sequentially and use deques, which keep element addresses stable like concurrent_vector.
#pragma once
#ifdef _MSC_VER
#include <ppl.h>
#include <concurrent_vector.h>
#else
#include <algorithm>
#include <deque>
namespace Concurrency {
template<class T> using concurrent_vector = std::deque<T>;
template<class Iterator, class Function> void parallel_for_each(Iterator first, Iterator last, Function function)
{
    std::for_each(first, last, function);
}
}
#endif
