#ifndef __core_features_h__
#define __core_features_h__

// Variant based
#if defined(DEBUG) && !defined(feature_TRACE)

#define feature_TRACE

#endif

// Compiler specific
#if defined(__GNUC__)
#define feature_GCC
#endif

#if defined(_MSC_VER)
#define feature_MSVC
#endif

// Platform specific
#if defined(__linux__)

#define feature_POSIX
#define feature_PTHREADS

#elif defined(_WIN32)

#define feature_WIN32

#else
#error "Unsupported platform"
#endif

#endif
