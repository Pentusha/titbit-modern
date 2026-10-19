#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
extern void WAPI SetLastError(uint32_t);
typedef struct Heap Heap;typedef union Block Block;
union Block {max_align_t alignment;struct {Heap *heap;Block *previous,*next;size_t size;} data;};
struct Heap {uint32_t magic;pthread_mutex_t mutex;size_t maximum,total;Block *first;};
#define HEAP_MAGIC 0x54424850u
void *WAPI HeapCreate(uint32_t flags,size_t initial,size_t maximum){
 (void)initial;if(flags&~1u){SetLastError(87);return NULL;}Heap *heap=calloc(1,sizeof(*heap));if(!heap){SetLastError(8);return NULL;}
 heap->magic=HEAP_MAGIC;heap->maximum=maximum;pthread_mutex_init(&heap->mutex,NULL);return heap;
}
void *WAPI HeapAlloc(Heap *heap,uint32_t flags,size_t size){
 if(!heap||heap->magic!=HEAP_MAGIC||(flags&~9u)||size>SIZE_MAX-sizeof(Block)){SetLastError(87);return NULL;}
 pthread_mutex_lock(&heap->mutex);if(heap->maximum&&(size>heap->maximum||heap->total>heap->maximum-size)){pthread_mutex_unlock(&heap->mutex);SetLastError(8);return NULL;}
 Block *block=malloc(sizeof(*block)+size);if(!block){pthread_mutex_unlock(&heap->mutex);SetLastError(8);return NULL;}
 block->data=(typeof(block->data)){heap,NULL,heap->first,size};if(heap->first)heap->first->data.previous=block;heap->first=block;heap->total+=size;
 pthread_mutex_unlock(&heap->mutex);if(flags&8u)memset(block+1,0,size);return block+1;
}
int32_t WAPI HeapFree(Heap *heap,uint32_t flags,void *pointer){
 if(!pointer)return 1;
 if(!heap||heap->magic!=HEAP_MAGIC||(flags&~1u)){SetLastError(87);return 0;}
 Block *block=(Block*)pointer-1;if(block->data.heap!=heap){SetLastError(87);return 0;}
 pthread_mutex_lock(&heap->mutex);if(block->data.previous)block->data.previous->data.next=block->data.next;else heap->first=block->data.next;
 if(block->data.next)block->data.next->data.previous=block->data.previous;
 heap->total-=block->data.size;pthread_mutex_unlock(&heap->mutex);free(block);return 1;
}
size_t WAPI HeapSize(Heap *heap,uint32_t flags,const void *pointer){(void)flags;if(!heap||heap->magic!=HEAP_MAGIC||!pointer){SetLastError(87);return SIZE_MAX;}const Block *block=(const Block*)pointer-1;if(block->data.heap!=heap){SetLastError(87);return SIZE_MAX;}return block->data.size;}
void *WAPI HeapReAlloc(Heap *heap,uint32_t flags,void *pointer,size_t size){
 if(!pointer)return HeapAlloc(heap,flags,size);
 size_t old=HeapSize(heap,0,pointer);if(old==SIZE_MAX)return NULL;
 if(flags&0x10u){if(size<=old)return pointer;SetLastError(8);return NULL;}
 if((flags&~9u)||size>SIZE_MAX-sizeof(Block)){SetLastError(87);return NULL;}
 pthread_mutex_lock(&heap->mutex);
 if(heap->maximum&&(size>heap->maximum||heap->total-old>heap->maximum-size)){pthread_mutex_unlock(&heap->mutex);SetLastError(8);return NULL;}
 Block *block=(Block*)pointer-1;Block *replacement=realloc(block,sizeof(*block)+size);
 if(!replacement){pthread_mutex_unlock(&heap->mutex);SetLastError(8);return NULL;}
 if(replacement->data.previous)replacement->data.previous->data.next=replacement;else heap->first=replacement;
 if(replacement->data.next)replacement->data.next->data.previous=replacement;
 replacement->data.size=size;heap->total=heap->total-old+size;pthread_mutex_unlock(&heap->mutex);
 if((flags&8u)&&size>old)memset((char*)(replacement+1)+old,0,size-old);
 return replacement+1;
}
int32_t WAPI HeapDestroy(Heap *heap){
 if(!heap||heap->magic!=HEAP_MAGIC){SetLastError(6);return 0;}
 pthread_mutex_lock(&heap->mutex);Block *block=heap->first;while(block){Block *next=block->data.next;free(block);block=next;}heap->magic=0;pthread_mutex_unlock(&heap->mutex);pthread_mutex_destroy(&heap->mutex);free(heap);return 1;
}
