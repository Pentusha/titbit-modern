struct Base {
    int x;
    Base();
    virtual ~Base();
    virtual int value(int);
};
Base::Base() {x=31;}
Base::~Base() {}
int Base::value(int y) {return x+y;}
int plain(int x,int y) {return x+y;}
extern "C" int plain_c(int x,int y) {return x+y;}
extern "C" __cdecl int explicit_c(int x,int y) {return x+y;}
int call(Base* b) {return b->value(2)+plain(3,4)+plain_c(5,6)+explicit_c(7,8);}
