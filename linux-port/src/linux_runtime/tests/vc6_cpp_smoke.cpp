// Execute the installed-header ABI against the authored native runtime.
#include <string>
#include <locale>
#include <stdexcept>
#include <cstdio>
#include <fstream>
#include <io.h>

struct UnwindObserver {
 bool *observed;
 explicit UnwindObserver(bool *value) : observed(value) {}
 ~UnwindObserver() { *observed = std::uncaught_exception(); }
};

extern "C" int titbit_vc6_cpp_smoke() {
 std::string first("hello"), second(first);
 second += " world";
 if (first != "hello" || second != "hello world") return 1;
 second.replace(0, 5, "native");
 if (second != "native world") return 2;
 bool caught = false;
 try { first.at(100); }
 catch (const std::out_of_range& error) { caught = error.what()[0] != 0; }
 if (!caught) return 3;
 const char *message = "message copy";
 exception error(message), copied(error);
 if (strcmp(error.what(), copied.what())) return 4;
 bool unwinding = false;
 try { UnwindObserver observe(&unwinding); throw 42; }
 catch (int value) { if (value != 42) return 5; }
 if (!unwinding || std::uncaught_exception()) return 6;
 std::locale locale;
 const std::ctype<char>& facet = std::use_facet(locale, (std::ctype<char> *)0, true);
 if (!facet.is(std::ctype_base::alpha, 'A') || facet.is(std::ctype_base::digit, 'A')) return 7;
 if (facet.tolower('Q') != 'q' || facet.toupper('x') != 'X') return 8;
 {
  std::ofstream file("build/vc6-runtime-smoke-data.txt", std::ios_base::binary | std::ios_base::trunc);
  if (!file) return 9;
  file << "native-stream\n";
  if (!file.good()) return 10;
 }
 {
  std::ifstream file("build/vc6-runtime-smoke-data.txt", std::ios_base::binary);
  std::string word;
  file >> word;
  if (word != "native-stream") return 11;
  file.clear();
  file.seekg(0, std::ios_base::beg);
  if (file.get() != 'n') return 12;
 }
 remove("build/vc6-runtime-smoke-data.txt");
 FILE *file = fopen("build/vc6-runtime-cstdio.bin", "w+b");
 if (!file) return 20;
 const char payload[] = "abcdef\n";
 if (fwrite(payload, 1, 7, file) != 7 || fflush(file) != 0) return 21;
 if (_fileno(file) < 0 || _filelength(_fileno(file)) != 7) return 22;
 if (fseek(file, 0, SEEK_SET) || getc(file) != 'a' || getc(file) != 'b') return 23;
 if (ungetc('z', file) != 'z' || getc(file) != 'z') return 24;
 if (fseek(file, 0, SEEK_SET)) return 25;
 char bytes[32];
 if (fread(bytes, 1, sizeof(bytes), file) != 7 || memcmp(bytes, payload, 7) || !feof(file)) return 26;
 if (fseek(file, 0, SEEK_SET) || feof(file) || ferror(file)) return 27;
 fpos_t desired = ((fpos_t)1 << 32) + 37, actual = -1;
 if (fsetpos(file, &desired) || fgetpos(file, &actual) || actual != desired) return 28;
 if (fseek(file, 0, SEEK_SET) || fgetpos(file, &actual) || actual != 0) return 29;
 if (fclose(file)) return 30;
 remove("build/vc6-runtime-cstdio.bin");
 return 0;
}
