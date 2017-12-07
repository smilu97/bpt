
/*
 *  Author: smilu97, Gim Yeongjin
 */

#ifndef __DBPT_H__
#define __DBPT_H__

#include "page.h"
#include "transaction.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

#define LEAF_SPLIT_TOLERANCE     (LEAF_KEYVALUE_NUM - 10)
#define INTERNAL_SPLIT_TOLERANCE (INTERNAL_KEYVALUE_NUM - 10)

#define LEAF_MERGE_TOLERANCE (LEAF_KEYVALUE_NUM / 2)
#define INTERNAL_MERGE_TOLERANCE (INTERNAL_KEYVALUE_NUM / 2)

void print_tree(int table_id);
void print_all();

MemoryPage * get_root();
int height(MemoryPage * root);
void find_and_print_range(int table_id, llu left, llu right);
MemoryPage * find_leaf(int table_id, llu key);
MemoryPage * find_first_leaf(int table_id);
MemoryPage * find_left(MemoryPage * m_leaf);
MemoryPage * find_right(MemoryPage * m_leaf);
char * find_from_leaf(MemoryPage * m_leaf, llu key);
char * find(int table_id, llu key);

int destroy_tree(int table_id);

int insert(int table_id, llu key, const char * value);
int insert_into_leaf(MemoryPage * m_leaf, llu key, const char * value);
int insert_into_leaf_after_splitting(MemoryPage * m_leaf, llu key, const char * value);
int insert_into_parent(MemoryPage * m_left, llu new_key, MemoryPage * m_new_leaf);
llu get_left_idx(InternalPage * parent, llu leftOffset);
int insert_into_internal(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * right);
int insert_into_internal_after_splitting(MemoryPage * m_internal, llu left_idx, llu new_key, MemoryPage * right);
int insert_into_new_root(MemoryPage * left, llu key, MemoryPage * right);

int delete(int table_id, llu key);
int delete_leaf_entry(MemoryPage * m_leaf, llu key);

llu change_key_in_parent(MemoryPage * m_page, llu key);
int coalesce_leaf(MemoryPage * m_left, MemoryPage * m_right);
int redistribute_leaf(MemoryPage * m_left, MemoryPage * m_right);
int coalesce_internal(MemoryPage * m_left, MemoryPage * m_right);
int redistribute_internal(MemoryPage * m_left, MemoryPage * m_right);

int update(int table_id, llu key, char * value);

int join_table(int table_id_1, int table_id_2, char * pathname);

extern MemoryPage * page_buf[MEMPAGE_MOD];
extern MemoryPage * mempages;
extern MemoryPage * free_mempage;
extern int mempage_num;
extern int last_mempage_idx;
extern int MAX_MEMPAGE;

#endif
