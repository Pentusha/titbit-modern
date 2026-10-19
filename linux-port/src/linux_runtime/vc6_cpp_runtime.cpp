// Authored native support for the VC6 header ABI. Compile against the user's
// VC98 headers; no SDK library objects or copied implementation sources.
#include <exception>
#include <stdexcept>
#include <xlocale>
#include <xiosbase>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>

extern "C" bool titbit_native_uncaught_exception();
extern "C" unsigned short *tb_crt_pctype;

static char *copy_exception_message(const char *message) {
 if (!message) return 0;
 unsigned length = (unsigned)strlen(message) + 1;
 char *copy = (char *)malloc(length);
 if (copy) memcpy(copy, message, length);
 return copy;
}

exception::exception() : _m_what(0), _m_doFree(0) {}
exception::exception(const __exString& message)
 : _m_what(copy_exception_message(message)), _m_doFree(_m_what != 0) {}
exception::exception(const exception& other)
 : _m_what(copy_exception_message(other._m_what)), _m_doFree(_m_what != 0) {}
exception& exception::operator=(const exception& other) {
 if (this != &other) {
  char *replacement = copy_exception_message(other._m_what);
  if (_m_doFree) free((void *)_m_what);
  _m_what = replacement;
  _m_doFree = replacement != 0;
 }
 return *this;
}
exception::~exception() {
 if (_m_doFree) free((void *)_m_what);
}
__exString exception::what() const {
 return _m_what ? _m_what : "Unknown exception";
}

namespace std {
bool __cdecl uncaught_exception() { return titbit_native_uncaught_exception(); }
void __cdecl _Xlen() { throw length_error("string too long"); }
void __cdecl _Xran() { throw out_of_range("invalid string position"); }

static bool supported_locale(const char *name) {
 return !name || !*name || !strcmp(name, "C") || !strcmp(name, "POSIX");
}

// The engine and XML parser use the byte-oriented C locale. Reject unsupported
// named locales explicitly rather than interpreting Windows LCID as a POSIX one.
_Locinfo::_Locinfo(const char *name) : _Oname("C"), _Nname("C") {
 if (!supported_locale(name)) throw runtime_error("unsupported locale name");
}
_Locinfo::_Locinfo(int, const char *name) : _Oname("C"), _Nname("C") {
 if (!supported_locale(name)) throw runtime_error("unsupported locale name");
}
_Locinfo::~_Locinfo() {}
_Locinfo& _Locinfo::_Addcats(int, const char *name) {
 if (!supported_locale(name)) throw runtime_error("unsupported locale name");
 return *this;
}

int locale::id::_Id_cnt = 0;
locale::id ctype<char>::id;
const ctype<char>::mask *ctype<char>::_Cltab = 0;
const size_t ctype<char>::table_size = 256;
locale::_Locimp *locale::_Locimp::_Clocptr = 0;
locale::_Locimp *locale::_Locimp::_Global = 0;

locale::_Locimp::_Locimp(bool transparent)
 : facet(1), _Fv(0), _Nfv(0), _Cat(all), _Xpar(transparent), _Name(transparent ? "*" : "C") {}
locale::_Locimp::_Locimp(const _Locimp& other)
 : facet(1), _Fv(0), _Nfv(other._Nfv), _Cat(other._Cat), _Xpar(other._Xpar), _Name(other._Name) {
 if (_Nfv) {
  _Fv = new facet *[_Nfv];
  for (size_t i = 0; i < _Nfv; ++i) {
   _Fv[i] = other._Fv[i];
   if (_Fv[i]) _Fv[i]->_Incref();
  }
 }
}
locale::_Locimp::~_Locimp() {
 for (size_t i = 0; i < _Nfv; ++i) if (_Fv[i]) delete _Fv[i]->_Decref();
 delete[] _Fv;
}
void locale::_Locimp::_Addfac(facet *value, size_t id) {
 if (id >= _Nfv) {
  facet **replacement = new facet *[id + 1];
  for (size_t i = 0; i <= id; ++i) replacement[i] = i < _Nfv ? _Fv[i] : 0;
  delete[] _Fv;
  _Fv = replacement;
  _Nfv = id + 1;
 }
 if (value) value->_Incref();
 if (_Fv[id]) delete _Fv[id]->_Decref();
 _Fv[id] = value;
}
locale::_Locimp *locale::_Init() {
 if (!_Locimp::_Clocptr) {
  _Locimp::_Clocptr = new _Locimp(false);
  _Locimp::_Global = _Locimp::_Clocptr;
 }
 return _Locimp::_Global;
}
locale& locale::_Addfac(facet *value, size_t id, size_t category) {
 _Locimp *replacement = new _Locimp(*_Ptr);
 try { replacement->_Addfac(value, id); replacement->_Name = "*"; }
 catch (...) { delete replacement; throw; }
 delete _Ptr->_Decref();
 _Ptr = replacement;
 if (category < 32) _Ptr->_Cat |= (1u << category) >> 1;
 return *this;
}

const locale::facet *locale::_Getfacet(size_t id, bool global_fallback) const {
 if (_Ptr && id < _Ptr->_Nfv && _Ptr->_Fv[id]) return _Ptr->_Fv[id];
 if (global_fallback && _Ptr && _Ptr->_Xpar && _Locimp::_Global &&
     id < _Locimp::_Global->_Nfv) return _Locimp::_Global->_Fv[id];
 // VC6 use_facet creates and retains the classic facet when this returns null.
 return 0;
}
bool locale::_Iscloc() const {
 return !_Ptr || _Ptr->_Name.compare("C") == 0 || _Ptr->_Name.compare("POSIX") == 0;
}

int ios_base::_Index = 0;
bool ios_base::_Sync = true;
const fpos_t _Fpz = 0;

void ios_base::_Init() {
 _State = goodbit;
 _Except = goodbit;
 _Fmtfl = skipws | dec;
 _Prec = 6;
 _Wide = 0;
 _Arr = 0;
 _Calls = 0;
 new (&_Loc) locale();
}
ios_base::~ios_base() {
 for (_Fnarray *p = _Calls; p; p = p->_Next) p->_Pfn(erase_event, *this, p->_Index);
 while (_Calls) { _Fnarray *p = _Calls; _Calls = p->_Next; delete p; }
 while (_Arr) { _Iosarray *p = _Arr; _Arr = p->_Next; delete p; }
}

void ios_base::clear(iostate state, bool rethrow_current) {
 _State = state & _Statmask;
 if ((_State & _Except) != 0) {
  if (rethrow_current) throw;
  throw failure("stream state error");
 }
}

FILE *__cdecl __Fiopen(const char *name, ios_base::openmode flags) {
 const char *mode;
 bool read = (flags & ios_base::in) != 0;
 bool write = (flags & ios_base::out) != 0;
 bool append = (flags & ios_base::app) != 0;
 bool truncate = (flags & ios_base::trunc) != 0;
 if (append) write = true;
 if ((!read && !write) || (truncate && !write)) return 0;
 if (append) mode = read ? "a+b" : "ab";
 else if (truncate || (write && !read)) mode = read ? "w+b" : "wb";
 else mode = write ? "r+b" : "rb";
 FILE *file = fopen(name, mode);
 if (file && (flags & ios_base::ate) && fseek(file, 0, SEEK_END)) { fclose(file); file = 0; }
 return file;
}
}

extern "C" _Ctypevec __cdecl _Getctype() {
 _Ctypevec result;
 short *table = (short *)malloc(256 * sizeof(short));
 if (!table) throw std::bad_alloc();
 memcpy(table, tb_crt_pctype, 256 * sizeof(short));
 result._Hand = 0;
 result._Page = 0;
 result._Table = table;
 result._Delfl = 1;
 return result;
}
extern "C" int __cdecl _Tolower(int value, const _Ctypevec *) {
 return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
}
extern "C" int __cdecl _Toupper(int value, const _Ctypevec *) {
 return value >= 'a' && value <= 'z' ? value - ('a' - 'A') : value;
}
extern "C" _Cvtvec __cdecl _Getcvt() {
 _Cvtvec value = {0, 0};
 return value;
}
