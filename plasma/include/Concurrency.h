#pragma once
#ifdef _MSC_VER
#include <concurrent_vector.h>
#include <ppl.h>
#else
#include <algorithm>
#include <deque>
namespace Concurrency {
// Subdivision allocations need stable element addresses. The non-MSVC
// implementation executes each subdivision pass serially.
template<class T> using concurrent_vector = std::deque<T>;
template<class Iterator, class Function> void parallel_for_each(Iterator first, Iterator last, Function function)
{
    std::for_each(first, last, function);
}
}
#endif
