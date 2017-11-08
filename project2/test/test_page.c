
/*
 * Author: smilu97
 */

#include "tests.h"

#define PAGE_NUM 1000

int test_page()
{
    MemoryPage *m_a, *m_b;
    Page *a, *b;

    init_db(1000);

    unlink("db/test_page.db");
    int fd = open_table("db/test_page.db");

    char buf[200];

    for(int i=1; i<PAGE_NUM; ++i) {
        Page * page = get_page(fd, i)->p_page;
        sprintf(page->bytes, "Hello I'm record %d", i);
        commit_page(fd, page, i, 200, 0);
    }

    for(int i=1; i<PAGE_NUM; ++i) {
        Page * page = get_page(fd, i)->p_page;
        sprintf(buf, "Hello I'm record %d", i);
        if(strcmp(buf, page->bytes) != 0) {
            printf("buf  : %s\n", buf);
            printf("page : %s\n", page->bytes);
            return false;
        }
    }

    printf("InternalPageHeader  size: %lu\n", sizeof(InternalPageHeader));
    printf("LeafPageHeader      size: %lu\n", sizeof(LeafPageHeader));
    printf("Record              size: %lu\n", sizeof(Record));
    printf("HeaderPage          size: %lu\n", sizeof(HeaderPage));
    printf("InternalPage        size: %lu\n", sizeof(InternalPage));
    printf("LeafPage            size: %lu\n", sizeof(LeafPage));
    printf("FreePage            size: %lu\n", sizeof(FreePage));
    printf("Internal Split Tolerance: %lu\n", INTERNAL_SPLIT_TOLERANCE);
    printf("Leaf     Split Tolerance: %lu\n", LEAF_SPLIT_TOLERANCE);
    printf("Internal Merge Tolerance: %lu\n", INTERNAL_MERGE_TOLERANCE);
    printf("Leaf     Merge Tolerance: %lu\n", LEAF_MERGE_TOLERANCE);
    
    puts("test page success");

    close_table(fd);
    shutdown_db();

    // look at test.db
    return true;
}
