#ifndef __core_features_h__
#define __core_features_h__

#if defined(__linux__)

#define feature_PTHREADS

#elif defined(WIN32)

#else
#error "Unsupported platform"
#endif

