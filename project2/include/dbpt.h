
/*
 *  Author: smilu97, Gim Yeongjin
 */

#ifndef __DBPT_H__
#define __DBPT_H__

#include "page.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define LEAF_SPLIT_TOLERANCE     (LEAF_KEYVALUE_NUM - 10)
#define INTERNAL_SPLIT_TOLERANCE (INTERNAL_KEYVALUE_NUM - 10)

#define LEAF_MERGE_TOLERANCE (LEAF_KEYVALUE_NUM / 2)
#define INTERNAL_MERGE_TOLERANCE (INTERNAL_KEYVALUE_NUM / 2)

void print_tree();

MemoryPage * get_root();
int height(Page * root);
MemoryPage * find_leaf(llu key);
MemoryPage * find_first_leaf();
MemoryPage * find_left(MemoryPage * m_leaf);
char * find(llu key);

int insert(llu key, const char * value);
int insert_into_leaf(MemoryPage * m_leaf, llu key, const char * value);
int insert_into_leaf_after_splitting(MemoryPage * m_leaf, llu key, const char * value);
int insert_into_parent(MemoryPage * m_left, llu new_key, MemoryPage * m_new_leaf);
llu get_left_idx(InternalPage * parent, llu leftOffset);
int insert_into_internal(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * right);
int insert_into_internal_after_splitting(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * right);
int insert_into_new_root(MemoryPage * left, llu key, MemoryPage * right);

void print_all();

int delete(llu key);

#endif
