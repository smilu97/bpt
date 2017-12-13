
/*
 * Author: smilu97
 */

#include "tests.h"

int found[1000];
int found_idx = 0;
int fail[1000];
int fail_idx = 0;

#define TEST_BUF   (400000)
#define INSERT_NUM (10000)

extern unsigned long long pinned_page_num;

int test_insert_sequence()
{
    struct timespec begin, end;

    unlink("db/test_insert_seq.db");
    
    init_db(TEST_BUF);
    int table_id = open_table("db/test_insert_seq.db");

    char buf[200];
    
    printf("Insert sequence start(%d)\n", INSERT_NUM);
    clock_gettime(CLOCK_MONOTONIC, &begin);
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", i);
        insert(table_id, i, buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double total = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0f;
    printf("insert time: %f\n", total);

    int success = 1;
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", i);
        char * record = find(table_id, i);
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

    close_table(table_id);
    shutdown_db();
    return true;
}

int test_insert_random()
{
    struct timespec begin, end;
    unlink("db/test_insert_random.db");

    init_db(TEST_BUF);
    int table_id = open_table("db/test_insert_random.db");

    char buf[200];
    int indices[INSERT_NUM];
    for(int i=0; i<INSERT_NUM; ++i) indices[i] = i;
    srand((unsigned int)time(0));
    shuffle_d(indices, INSERT_NUM);

    printf("Insert random start(%d)\n", INSERT_NUM);

    clock_gettime(CLOCK_MONOTONIC, &begin);
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", indices[i]);
        insert(table_id, indices[i], buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double total = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0f;

    printf("insert time: %f\n", total);

    int success = 1;
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", indices[i]);
        char * record = find(table_id, indices[i]);
        
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
    
    close_table(table_id);
    shutdown_db();
    return true;
}
