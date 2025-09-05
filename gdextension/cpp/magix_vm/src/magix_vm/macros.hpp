#ifndef MAGIX_MACROS_HPP_
#define MAGIX_MACROS_HPP_

#if __cplusplus >= 201703L
#define MAGIX_CXX17 1
#endif

#if __cplusplus >= 202002L
#define MAGIX_CXX20 1
#endif

#if __cplusplus >= 202302L
#define MAGIX_CXX23 1
#endif

#ifdef MAGIX_CXX17
#define MAGIX_CONSTEXPR_CXX17(...) constexpr
#else
#define MAGIX_CONSTEXPR_CXX17(...)
#endif

#ifdef MAGIX_CXX20
#define MAGIX_CONSTEXPR_CXX20(...) constexpr
#else
#define MAGIX_CONSTEXPR_CXX20(...)
#endif

#ifdef MAGIX_CXX23
#define MAGIX_CONSTEXPR_CXX23(...) constexpr
#else
#define MAGIX_CONSTEXPR_CXX23(...)
#endif

#endif // MAGIX_MACROS_HPP_
