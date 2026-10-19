// GNU C++ ABI boundary for the Microsoft-ABI header runtime.
#include <exception>
extern "C" bool titbit_native_uncaught_exception() {
 return std::uncaught_exception();
}
