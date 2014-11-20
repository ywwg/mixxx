#ifndef MATH_H
#define MATH_H

// Causes MSVC to define M_PI and friends.
// http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include <QtDebug>

// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
using std::fabs;

#define math_max std::max
#define math_min std::min
#define math_max3(a, b, c) math_max(math_max((a), (b)), (c))

// Restrict value to the range [min, max]. Undefined behavior
// if min > max.
template <typename T>
inline T math_clamp_unsafe(T value, T min, T max) {
    return math_max(min, math_min(max, value));
}

// Clamp with bounds checking to avoid undefined behavior
// on invalid min/max parameters.
template <typename T>
inline T math_clamp_safe(T value, T min, T max) {
    if (min <= max) {
        // valid bounds
        return math_clamp_unsafe(value, min, max);
    } else {
        // invalid bounds
        qWarning() << "PROGRAMMING ERROR: math_clamp_safe() called with min > max!"
                   << min << ">" << max;
        // simply return the value unchanged
        return value;
    }
}

template <typename T>
inline T math_clamp(T value, T min, T max) {
    return math_clamp_safe(value, min, max);
}

// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
inline bool even(T value) {
    return value % 2 == 0;
}

#ifdef _MSC_VER
// VC++ uses _isnan() instead of isnan() and !_finite instead of isinf.
#include <float.h>
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
// Ask VC++ to emit an intrinsic for fabs instead of calling std::fabs.
#pragma intrinsic(fabs)
#else
// for isnan() and isinf() everywhere else use the cmath version. We define
// these as macros to prevent clashing with c++11 built-ins in the global
// namespace. If you say "using std::isnan;" then this will fail to build with
// std=c++11. See https://bugs.webkit.org/show_bug.cgi?id=59249 for some
// relevant discussion.
#define isnan std::isnan
#define isinf std::isinf
#endif

inline int roundUpToPowerOf2(int v) {
    // From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

// MSVS 2013 (_MSC_VER 1800) introduced C99 support.
#if defined(__WINDOWS__) &&  _MSC_VER < 1800
inline int round(double x) {
    return x < 0.0 ? ceil(x - 0.5) : floor(x + 0.5);
}
#endif

template <typename T>
inline const T ratio2db(const T a) {
    return log10(a) * 20;
}

template <typename T>
inline const T db2ratio(const T a) {
    return pow(10, a / 20);
}

#endif /* MATH_H */
