
/*
 * Author: smilu97
 */

#include "tests.h"

int found[1000];
int found_idx = 0;
int fail[1000];
int fail_idx = 0;

#define TEST_BUF (1000)
#define INSERT_NUM (1000000)

extern unsigned long long pinned_page_num;

int test_insert_sequence()
{
    unlink("db/test_insert_seq.db");
    
    init_db(TEST_BUF);
    int fd = open_table("db/test_insert_seq.db");

    char buf[200];
    
    printf("Insert sequence start(%d)\n", INSERT_NUM);

    int start = time(0);
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", i);
        insert(fd, i, buf);
    }
    int end = time(0); 
    printf("insert time: %d\n", end - start);

    int success = 1;
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", i);
        char * record = find(fd, i);
        if(record == NULL || strcmp(record, buf) != 0) {
            printf("record: %s\nbuf: %s\n", record, buf);
            success = 0;
            break;
        }
    }
    if(success) {
        puts("test insert sequence success");
    } else {
        myerror("sequence validation fail");
    }

    close_table(fd);
    shutdown_db();
    return true;
}

int test_insert_random()
{
    unlink("db/test_insert_random.db");

    init_db(TEST_BUF);
    int fd = open_table("db/test_insert_random.db");

    char buf[200];
    int indices[INSERT_NUM];

    srand((unsigned int)time(0));
    printf("Insert random start(%d)\n", INSERT_NUM);

    int start = time(0);
    for(int i=0; i<INSERT_NUM; ++i) {
        indices[i] = (int)(rand() % 0x7FFFFF);
        sprintf(buf, "I'm a record %d", indices[i]);
        insert(fd, indices[i], buf);
    }
    int end = time(0);
    printf("insert time: %d\n", end - start);

    int success = 1;
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", indices[i]);
        char * record = find(fd, indices[i]);
        
        if(record == NULL || strcmp(record, buf) != 0) {
            printf("record: %s\nbuf: %s\n", record, buf);
            success = 0;
            break;
        } 
    }
    if(success) {
        puts("test insert random success");
    } else {
        myerror("random validation failed");
    }
    
    close_table(fd);
    shutdown_db();
    return true;
}
