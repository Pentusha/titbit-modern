namespace std {class type_info;}
class bad_cast {public:virtual ~bad_cast() {}};
class Base {public:int x;virtual ~Base() {}};
class Derived:public Base {public:int y;};
class Left:public Base {public:int left;};
class Right:public Base {public:int right;};
class Both:public Left,public Right {};
class VLeft:virtual public Base {public:int left;};
class VRight:virtual public Base {public:int right;};
class Diamond:public VLeft,public VRight {};
class Private:private Base {public:Base *expose() {return this;}};
class VirtualRoot {public:virtual int value() {return 1;}};
class OnlyVirtual:virtual public VirtualRoot {public:int field;};
class Extra {public:virtual int other() {return 2;}};
class WithVirtual:public OnlyVirtual,public Extra {};
class Three:public Left,public Right,public Extra {};
extern "C" int probe_rtti() {
    Base b;
    Derived d;
    Base *p=&d;
    if(dynamic_cast<Derived *>(p)!=&d)return 1;
    p=&b;
    if(dynamic_cast<Derived *>(p)!=0)return 2;
    Both both;
    Left *left=&both;
    Right *right=&both;
    if(dynamic_cast<Right *>(left)!=right)return 3;
    if(dynamic_cast<void *>(right)!=&both)return 4;
    Base *repeated=left;
    if(dynamic_cast<Right *>(repeated)!=right)return 5;
    if(dynamic_cast<Both *>(repeated)!=&both)return 6;
    Diamond diamond;
    Base *shared=&diamond;
    if(dynamic_cast<Diamond *>(shared)!=&diamond)return 7;
    if(dynamic_cast<VRight *>(static_cast<VLeft *>(&diamond))!=static_cast<VRight *>(&diamond))return 8;
    Private priv;
    if(dynamic_cast<Private *>(priv.expose())!=0)return 9;
    try {
        p=&b;
        (void)dynamic_cast<Derived&>(*p);
        return 10;
    }catch(bad_cast&) {}
    p=&d;
    if(&typeid(*p)!=&typeid(Derived))return 11;
    WithVirtual with_virtual;
    OnlyVirtual *only=&with_virtual;
    if(dynamic_cast<WithVirtual *>(only)!=&with_virtual)return 12;
    if(dynamic_cast<Extra *>(only)!=static_cast<Extra *>(&with_virtual))return 13;
    if(&typeid(*only)!=&typeid(WithVirtual))return 14;
    Three three;
    if(dynamic_cast<Base *>(static_cast<Extra *>(&three))!=0)return 15;
    return 0;
}
void operator delete(void*) noexcept {}
