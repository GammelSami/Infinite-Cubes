// Compatibility wrappers for parallel loops and concurrent vectors. MSVC uses its Concurrency Runtime; other compilers
// execute loops sequentially and use standard vectors.
#pragma once
#ifdef _MSC_VER
#include <ppl.h>
#include <concurrent_vector.h>
#else
#include <algorithm>
#include <vector>
namespace Concurrency {
template<class T> using concurrent_vector = std::vector<T>;
template<class I, class F> void parallel_for_each(I first, I last, F f)
{
    std::for_each(first, last, f);
}
enum PolicyElementKey {
    ContextPriority
};
struct SchedulerPolicy {
    void SetConcurrencyLimits(unsigned, unsigned) {}
    void SetPolicyValue(PolicyElementKey, unsigned) {}
};
struct CurrentScheduler {
    static void Create(const SchedulerPolicy &) {}
};
}
#endif
