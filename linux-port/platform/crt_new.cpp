#include <new>
#include <exception>
extern "C" void *titbit_operator_new(unsigned size){return ::operator new(size);}
extern "C" void titbit_operator_delete(void *pointer){::operator delete(pointer);}
extern "C" int titbit_purecall(){std::terminate();}
