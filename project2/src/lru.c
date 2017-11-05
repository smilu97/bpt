
#include "lru.h"

void LRUClean() {
    LRUNode * cur = lru_head;
    while(cur != 0) {
        LRUNode * next = cur->next;
        free(cur);
        cur = next;
    }
}

void LRUInit() {
    lru_head = 0, lru_tail = 0;
}

LRUNode * LRUPush(void * mem)
{
    LRUNode * new_node = (LRUNode*)malloc(sizeof(LRUNode));
    new_node->mem = mem;

    if(lru_head == 0) {
        new_node->next = 0;
        new_node->prev = 0;

        lru_head = new_node;
        lru_tail = new_node;
    } else {
        lru_tail->next = new_node;
        
        new_node->next = 0;
        new_node->prev = lru_tail;
    
        lru_tail = new_node;
    }

    return new_node;
}

void * LRUPop()
{
    if(lru_head == 0) return 0;

    LRUNode * h = lru_head;

    if(lru_head->next) lru_head->next->prev = 0;
    else lru_tail = 0;
    lru_head = lru_head->next;

    void * res = h->mem;
    free(h);

    return res;
}

void LRUAdvance(LRUNode * node)
{
    if(node == lru_tail) return;

    if(node->prev) node->prev->next = node->next;
    if(node->next) node->next->prev = node->prev;

    lru_tail->next = node;
    node->prev = lru_tail;
    node->next = 0;

    lru_tail = node;
}
