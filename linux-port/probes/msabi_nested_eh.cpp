extern "C" {
int trace_value;
int error_copies;
int error_destroyed;
}
struct Guard {
    int id;
    Guard(int n):id(n) {}
    ~Guard() {trace_value=trace_value*10+id;}
};
struct Error {
    int code;
    Error(int n):code(n) {}
    Error(const Error& e):code(e.code) {++error_copies;}
    ~Error() {++error_destroyed;}
};
static int unmatched() {
    Guard g(6);
    try {throw Error(80);}
    catch(int n) {return n;}
}
extern "C" int probe_propagate() {
    trace_value=error_copies=error_destroyed=0;
    try {return unmatched();}
    catch(const Error& e) {return e.code+100;}
}
extern "C" int probe_by_value() {
    trace_value=error_copies=error_destroyed=0;
    try {throw Error(90);}
    catch(Error e) {return e.code;}
}
extern "C" int probe_throw_lvalue() {
    trace_value=error_copies=error_destroyed=0;
    try {Error local(91); throw local;}
    catch(const Error& e) {return e.code;}
}
extern "C" int probe_nested(int n) {
    trace_value=error_copies=error_destroyed=0;
    Guard outer(1);
    int result=0;
    try {
        Guard inner(2);
        try {
            Guard deep(3);
            if (n) throw Error(70);
            result=1;
        } catch(int x) {
            result=x;
        } catch(const Error& error) {
            Guard caught(4);
            if (n==2) throw;
            result=error.code;
        }
    } catch(const Error& error) {
        Guard caught(5);
        result=error.code+100;
    }
    return result;
}
