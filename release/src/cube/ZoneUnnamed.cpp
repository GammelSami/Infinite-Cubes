// Auxiliary zone state.
#include <list>

namespace cube {

// A list nothing in the program uses: only its dynamic initializer and its destructor refer to it.
std::list<long long> s_zones;

} // namespace cube
