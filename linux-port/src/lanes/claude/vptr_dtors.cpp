// Derived destructors that only restore their vptr and tail-call the shared
// base destructor 0x40b130.
struct B_0040b130 { virtual ~B_0040b130(); };
struct D_00415e30 : B_0040b130 { ~D_00415e30(); };
struct D_0042ca60 : B_0040b130 { ~D_0042ca60(); };
D_00415e30::~D_00415e30() {}
D_0042ca60::~D_0042ca60() {}
