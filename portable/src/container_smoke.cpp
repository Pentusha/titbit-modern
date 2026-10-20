// Cross-target assertions for layout and ownership at reconstructed boundaries.
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <list>
#include <stddef.h>
typedef char string_layout[sizeof(std::string)==16?1:-1];
typedef char vector_layout[sizeof(std::vector<int>)==16?1:-1];
typedef char deque_layout[sizeof(std::deque<void*>)==48?1:-1];
struct XmlAttributeLayout {std::string local,uri,qname,value,type;bool specified;};
typedef char attribute_layout[sizeof(XmlAttributeLayout)==84?1:-1];

extern "C" int titbit_portable_container_smoke() {
 std::string text("abcdef");
 text.assign(text.data()+2,3);
 if(text!="cde")return 1;
 text.append(text.data(),text.size());
 if(text!="cdecde")return 2;
 text.replace(1,3,text.data()+2,3);
 if(text!="cecdde")return 3;
 std::string empty;empty.replace(0,0,"",0);
 if(!empty.empty())return 4;
 std::vector<std::string> names;names.push_back("first");
 names.push_back(names.front());names.insert(names.begin(),names.back());
 if(names.size()!=3||names[0]!="first"||names[2]!="first")return 5;
 names.erase(names.begin(),names.begin()+2);
 if(names.size()!=1||names[0]!="first")return 6;
 std::deque<int> stack;stack.push_front(1);stack.push_front(2);stack.pop_front();
 if(stack.size()!=1||stack.front()!=1)return 7;
 std::multimap<std::string,std::string> context;
 context.insert(std::make_pair(std::string("z"),std::string("last")));
 context.insert(std::make_pair(std::string("a"),std::string("first")));
 context.insert(std::make_pair(std::string("a"),std::string("second")));
 if(context.begin()->first!="a"||context.find("a")->second!="first")return 8;
 std::vector<std::multimap<std::string,std::string> > contexts;
 contexts.push_back(context);context.clear();
 if(contexts.back().size()!=3||contexts.rbegin()->find("z")->second!="last")return 9;
 std::map<std::string,int> options;options["volume"]=42;
 if(options["volume"]!=42||options["missing"]!=0)return 10;
 std::list<int> values;values.push_back(1);values.push_back(2);values.push_back(1);values.remove(1);
 if(values.size()!=1||values.front()!=2)return 11;
 std::vector<int> counts(3,7);counts.insert(counts.begin(),2,counts.back());
 if(counts.size()!=5||counts[0]!=7||counts[4]!=7)return 12;
 std::deque<int> ring;for(int i=0;i<200;++i){ring.push_front(i);ring.push_back(-i);}
 for(int i=199;i>=0;--i){if(ring.front()!=i||ring.back()!=-i)return 13;ring.pop_front();ring.pop_back();}
 if(!ring.empty())return 14;
 return 0;
}
