#ifndef __core_features_h__
#define __core_features_h__

#if defined(__linux__)

#define feature_POSIX
#define feature_PTHREADS

#elif defined(_WIN32)

#define feature_WIN32

#else
#error "Unsupported platform"
#endif

#endif
