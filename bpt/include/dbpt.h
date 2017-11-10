
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

void print_tree(int table_id);

MemoryPage * get_root();
int height(MemoryPage * root);
MemoryPage * find_leaf(int table_id, llu key);
MemoryPage * find_first_leaf();
MemoryPage * find_left(MemoryPage * m_leaf);
char * find_from_leaf(MemoryPage * m_leaf, llu key);
char * find(int table_id, llu key);

int insert(int table_id, llu key, const char * value);
int insert_into_leaf(MemoryPage * m_leaf, llu key, const char * value);
int insert_into_leaf_after_splitting(MemoryPage * m_leaf, llu key, const char * value);
int insert_into_parent(MemoryPage * m_left, llu new_key, MemoryPage * m_new_leaf);
llu get_left_idx(InternalPage * parent, llu leftOffset);
int insert_into_internal(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * right);
int insert_into_internal_after_splitting(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * right);
int insert_into_new_root(MemoryPage * left, llu key, MemoryPage * right);

void print_all();

int delete(int table_id, llu key);
int delete_leaf_entry(MemoryPage * m_leaf, llu key);

int coalesce_leaf(MemoryPage * m_left, MemoryPage * m_right);
int redistribute_leaf(MemoryPage * m_left, MemoryPage * m_right);

extern MemoryPage * page_buf[MEMPAGE_MOD];
extern MemoryPage * mempages;
extern MemoryPage * free_mempage;
extern int mempage_num;
extern int last_mempage_idx;
extern int MAX_MEMPAGE;

#endif
