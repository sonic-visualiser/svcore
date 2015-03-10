
#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include <cstdint>

typedef int64_t sv_frame_t;
typedef double sv_samplerate_t;

template<typename T, typename C>
bool in_range_for(const C &container, T i)
{
    if (i < 0) return false;
    if (sizeof(T) > sizeof(typename C::size_type)) {
	return i < static_cast<T>(container.size());
    } else {
	return static_cast<typename C::size_type>(i) < container.size();
    }
}

#endif
