
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

#define MAX_MEMPAGE (10000000)
#define MEMPAGE_MOD (MAX_MEMPAGE)

#define DIRTY_QUEUE_SIZE 10000

#define FILE_OPEN_SETTING (O_RDWR | O_SYNC)

#define HEADER_PAGE_COMMIT_SIZE 24

typedef struct Page {
    BYTE bytes[PAGE_SIZE];
} Page;

/*
 *  Header page must be first page in file
 *  When we open the data file at first,
 *  initializing disk-based b+tree should be 
 *   done using this header page.
 */
typedef struct HeaderPage {
    offsetType freePageOffset;
    offsetType rootPageOffset;
    llu numOfPages;
    BYTE reserved[PAGE_SIZE - sizeof(llu) - (sizeof(offsetType) * 2)];
} HeaderPage;

/*
 *  Header page contains the first free page
 *
 *  Free pages are linked and allocation is
 *  managed by the free page list.
 */
typedef struct FreePage {
    offsetType nextOffset;  // 0, if end of the free page list.
    BYTE reserved[PAGE_SIZE - sizeof(offsetType)];
} FreePage;

/*
 *  type representing header of page
 *  Internal, Leaf page have first 128byte
 *  as a page header.
 */
typedef struct LeafPageHeader {
    offsetType parentOffset;
    lu isLeaf;
    lu numOfKeys;
    BYTE reserved[PAGE_HEADER_SIZE - (sizeof(offsetType) * 2) - (sizeof(lu) * 2)];
    offsetType rightOffset;
} LeafPageHeader;

typedef struct InternalPageHeader {
    offsetType parentOffset;
    lu isLeaf;
    lu numOfKeys;
    BYTE reserved[PAGE_HEADER_SIZE - (sizeof(offsetType) * 2) - (sizeof(lu) * 2)];
    offsetType oneMoreOffset;
} InternalPageHeader;

/*
 *  type representing serialized key and value
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

typedef struct Dirty {
    int left;
    int right;
} Dirty;

/*
 *  
 */
typedef struct MemoryPage {
    Dirty dirty;
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

void init_buf();
void delete_cache();
MemoryPage * get_page(llu page_num);
MemoryPage * new_page();
int free_page(llu idX);
int commit_page(Page * p_page, llu page_num, llu size, llu offset);
int load_page(Page * p_page, llu page_num, llu size);
int close_file();
int open_db(const char * filepath);
int set_parent(llu page_num, llu parent_num);
MemoryPage * find_hash_friend(MemoryPage * mem, llu page_num);

void make_free_mempage(llu idx);
MemoryPage * new_mempage();

Dirty make_dirty(int left, int right);
void update_dirty(Dirty * dest, Dirty src);
int register_dirty_page(MemoryPage * m_page, Dirty dirty);
int commit_dirty_pages();

/*******************************************************************
 * Global variables
 *******************************************************************/

int fd;
HeaderPage headerPage;
MemoryPage * page_buf[MEMPAGE_MOD];
MemoryPage * mempages;
MemoryPage * free_mempage;
int mempage_num;
int last_mempage_idx;

int dirty_queue[DIRTY_QUEUE_SIZE];
int dirty_queue_size;

#endif
