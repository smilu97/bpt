
#ifndef __PAGE_H__
#define __PAGE_H__

/*
*  Author: smilu97, Gim Yeongjin
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hash.h"
#include "lru.h"
#include "algo.h"

typedef signed   long long lld;
typedef unsigned long long llu;
typedef llu offsetType;

typedef int ld;
typedef unsigned lu;

typedef char BYTE;

/*
 *  Type representing page on disc.
 *  Its size must be 4096(0x1000)byte
 *  as it was described in specification
 *
 *  There are 4 types of page.
 *    1. Header page
 *    2. Free page
 *    3. Leaf page
 *    4. Internal page
 */

#define true  1
#define false 0

#define PAGE_SIZE        (4096)
#define KEY_SIZE         (8)
#define VALUE_SIZE       (120)
#define PAGE_HEADER_SIZE (128)

#define LEAF_KEYVALUE_NUM ((PAGE_SIZE - PAGE_HEADER_SIZE) / sizeof(Record))
#define INTERNAL_KEYVALUE_NUM ((PAGE_SIZE - PAGE_HEADER_SIZE) / (KEY_SIZE + sizeof(offsetType)))

/* The number of hash chain
 * When we find page in memory,
 * we use h(page_num) to find head of chain
 * 
 * MEMPAGE_MOD is the number of head of chains
 * 
 * As it has big number, memory page finding become faster
 */
#define MEMPAGE_MOD (1000000)

/* TODO: experiment for O_SYNC
 * When we open some files, always we use this option
 */
#define FILE_OPEN_SETTING (O_RDWR | O_SYNC)

/* The minimum size to commit header page
 * because the rest are reserved
 */
#define HEADER_PAGE_COMMIT_SIZE 24

/*
 */
#define PAGE_HEADER_COMMIT_SIZE 24

/* Representing size of container that save
 * the pinned memory pages
 */
#define PIN_CONTAINER_SIZE 100000

typedef struct Page {
    BYTE bytes[PAGE_SIZE];
} Page;

/*
 *  Header page must be first page in file
 *  When we open the data file at first,
 *  initializing disk-based b+tree should be 
 *  done using this header page.
 */
typedef struct HeaderPage {
    offsetType free_page_offset;
    offsetType root_page_offset;
    llu num_pages;
    llu page_lsn;
    BYTE reserved[PAGE_SIZE - (sizeof(llu) * 2) - (sizeof(offsetType) * 2)];
} HeaderPage;

/*
 *  Header page contains the first free page
 *
 *  Free pages are linked and allocation is
 *  managed by the free page list.
 */
typedef struct FreePage {
    offsetType next_offset;  // 0, if end of the free page list.
    BYTE reserved[PAGE_SIZE - sizeof(offsetType)];
} FreePage;

/*
 *  Type representing header of page
 *  Internal, Leaf page have first 128byte
 *  as a page header.
 */
typedef struct LeafPageHeader {
    offsetType parent_offset;
    lu is_leaf;
    lu num_keys;
    llu dummy1;
    llu page_lsn;
    BYTE reserved[PAGE_HEADER_SIZE - (sizeof(offsetType) * 2) - (sizeof(lu) * 2) - (sizeof(llu) * 2)];
    offsetType right_offset;
} LeafPageHeader;

typedef struct InternalPageHeader {
    offsetType parent_offset;
    lu is_leaf;
    lu num_keys;
    llu dummy1;
    llu page_lsn;
    BYTE reserved[PAGE_HEADER_SIZE - (sizeof(offsetType) * 2) - (sizeof(lu) * 2) - (sizeof(llu) * 2)];
    offsetType one_more_offset;
} InternalPageHeader;

/*
 *  Type representing serialized key and value
 */
typedef struct Record {
    llu key;
    BYTE value[VALUE_SIZE];
} Record;

typedef struct InternalKeyValue {
    llu key;
    offsetType offset;
} InternalKeyValue;

/*
 *  Leafpage contains the key/value records.
 *
 *  Keys are sorted in the page.
 * 
 *  One record size is 128 bytes and we contains
 *  maximum 31 records per page
 *
 *  First 128 bytes are used as a page header
 *
 *  Branch factor(order) = 32
 */
typedef struct LeafPage {
    LeafPageHeader header;
    Record keyValue[LEAF_KEYVALUE_NUM];
} LeafPage;

/*
 *  Internal page is similar to leaf page.
 *  it only contains 8byte value.
 *
 *  Branch factor(order) = 249
 *  (4096 - 128) / (8 + 8) = 248
 */
typedef struct InternalPage {
    InternalPageHeader header;
    InternalKeyValue keyValue[INTERNAL_KEYVALUE_NUM];
} InternalPage;

/* Dirty represents the range of dirty area in specific memory page
 *
 * Dirty means not being synced with disk
 * 
 * Dirty has next pointer to be in chain, and every dirty nodes in same chain
 * doesn't have any intersects
 * 
 * Dirty areas will be cleaned(synced with disk) in close_table or being free
 */
typedef struct Dirty {
    int left;
    int right;
    struct Dirty * next;
} Dirty;

/* MemoryPage is page in memory
 * 
 * It will be allocated when init_db called
 * the parameter buf_num of init_db(int) represents the number of MemoryPage
 * so, MemoryPages are allocated only in init_db(int) sizeof(MemoryPage) * buf_num
 * and, there will be sizeof(Page) * buf_num of space to save page content and
 * this space will be pointed by p_page in MemoryPages
 */
typedef struct MemoryPage {
    Dirty * dirty;
    int table_id;
    int pin_count;
    int pinned_idx;
    llu cache_idx;
    llu page_num;
    LRUNode * p_lru;
    Page * p_page;
    struct MemoryPage * next;
} MemoryPage;

/*******************************************************************
 * Functions
 *******************************************************************/

void describe_header(HeaderPage * head);
void describe_leaf(MemoryPage * m_leaf);
void describe_internal(MemoryPage * m_internal);

int is_same_file(int fd1, int fd2);

int init_db(int buf_num);
int open_table(const char * pathname);
int close_table(int table_id);
int shutdown_db(void);

MemoryPage * get_header_page(int table_id);
int init_buf();
void delete_cache();
MemoryPage * pop_unpinned_lru();
MemoryPage * get_page(int table_id, llu page_num);
MemoryPage * get_bufpage();
MemoryPage * new_page(int table_id);
int free_page(int table_id, llu page_num);
int commit_page(int table_id, Page * p_page, llu page_num, llu size, llu offset);
int load_page(int table_id, Page * p_page, llu page_num, llu size);
int close_file();
int set_parent(int table_id, llu page_num, llu parent_num);
MemoryPage * find_hash_friend(MemoryPage * mem, int table_id, llu page_num);

void make_free_mempage(llu idx);
MemoryPage * new_mempage(llu page_num, int table_id);

Dirty * make_dirty(int left, int right);
int register_dirty_page(MemoryPage * m_page, Dirty * dirty);

void register_pinned(MemoryPage * mem);
void unpin_all();
void unpin(MemoryPage * mem);

/*******************************************************************
 * Global variables
 *******************************************************************/


/* Import global variables from "lru.h"
 */
extern LRUNode * lru_head;
extern LRUNode * lru_tail;
extern llu lru_cnt;

/* Import global variables from "transaction.h"
 */
extern int log_fd;

/*
 * Include "transaction.h" in last because of cross-dependency problem
 */
#include "transaction.h"

#endif
