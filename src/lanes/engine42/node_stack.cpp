#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Arabica DOM builder helper: a std::deque of refcounted DOM node implementations
// (vtable 0x4cd73c) and the std::deque<T*> members it instantiates (0x42ea30-0x42ef30).
#pragma warning(disable: 4786)
#include <deque>

struct DomNodeImpl42 {
 virtual void s0();
 virtual void Release(int);                          // +0x04
 int refs;
};

class NodeStack_0042ea30 {
public:
 std::deque<DomNodeImpl42 *> nodes;
 NodeStack_0042ea30();
 virtual ~NodeStack_0042ea30();
 void Clear_0042ed80();
 void Push_0042eb70(DomNodeImpl42 *node);
};

NodeStack_0042ea30::NodeStack_0042ea30() {}

NodeStack_0042ea30::~NodeStack_0042ea30() {
 Clear_0042ed80();
}

// Releases every node, front to back.
void NodeStack_0042ea30::Clear_0042ed80() {
 while (nodes.size() != 0) {
  DomNodeImpl42 *node = nodes.front();
  if (--node->refs <= 0) node->Release(1);
  nodes.pop_front();
 }
}

// The node stack's push (deque::push_front expanded inline; `this` is the stack).
void NodeStack_0042ea30::Push_0042eb70(DomNodeImpl42 *node) {
 nodes.push_front(node);
}

template class std::deque<DomNodeImpl42 *>;
