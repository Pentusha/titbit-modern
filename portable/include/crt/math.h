#ifndef TITBIT_PORTABLE_MATH_H
#define TITBIT_PORTABLE_MATH_H
#include <titbit_crt.h>
#define HUGE_VAL (__builtin_huge_val())
TB_EXTERN_C_BEGIN
double sin(double);double cos(double);double tan(double);double asin(double);double acos(double);double atan(double);double atan2(double,double);
double sinh(double);double cosh(double);double tanh(double);double exp(double);double log(double);double log10(double);double pow(double,double);
double sqrt(double);double fabs(double);double floor(double);double ceil(double);double fmod(double,double);double modf(double,double*);
double frexp(double,int*);double ldexp(double,int);int _finite(double);int _isnan(double);double _hypot(double,double);
TB_EXTERN_C_END
#define isnan(x) __builtin_isnan(x)
#define isfinite(x) __builtin_isfinite(x)
#endif
