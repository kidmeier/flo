#ifndef __core_features_h__
#define __core_features_h__

// Variant based
#if defined(DEBUG) && !defined(feature_DEBUG)

#define feature_DEBUG

#endif
#if defined(TRACE) && !defined(feature_TRACE)

#define feature_TRACE

#elif defined(RELEASE) && !defined(feature_RELEASE)

#define feature_RELEASE

#endif

// Compiler specific
#if defined(__GNUC__)

#define feature_GCC

#define feature_TLS
#define threadlocal __thread

#elif defined(_MSC_VER)

#define feature_MSVC

#define feature_TLS
#define threadlocal __declspec( thread )

#endif

// Platform specific
#if defined(__linux__)

#define feature_GLX
#define feature_POSIX
#define feature_PTHREADS
#define feature_X11

#elif defined(_WIN32)

#define feature_WIN32

#else
#error "Unsupported platform"
#endif

#endif
