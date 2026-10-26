#pragma once
#ifdef _MSC_VER
#include "plasma/Parallel.h"
#include "plasma/Parallel.h"
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
