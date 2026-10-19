#include "engine_string.h"
ObservedString::ObservedString(const ObservedString&other){
 if(other.Header()->refs>=0){data=other.data;InterlockedIncrement(&Header()->refs);}
 else{Init();*this=other.data;}
}
