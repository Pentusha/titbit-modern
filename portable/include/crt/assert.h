#ifndef TITBIT_PORTABLE_ASSERT_H
#define TITBIT_PORTABLE_ASSERT_H
#include <stdlib.h>
#ifdef NDEBUG
#define assert(x) ((void)0)
#else
#define assert(x) ((x)?(void)0:abort())
#endif
#define _ASSERTE assert
#define _ASSERT assert
#endif
