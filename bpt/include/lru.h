
#ifndef __LRU_H__
#define __LRU_H__

#include <stdlib.h>

typedef struct LRUNode {
    void * mem;
    struct LRUNode * next;
    struct LRUNode * prev;
} LRUNode;

void LRUClean();
void LRUInit();
LRUNode * LRUPush(void * mem);
void LRUAdvance(LRUNode * node);
void * LRUPop();

#endif
