
/*
 * Author: smilu97
 */

#include "tests.h"

int found[1000];
int found_idx = 0;
int fail[1000];
int fail_idx = 0;

#define INSERT_NUM (10000000)

int test_insert_sequence()
{
    unlink("db/test_insert_seq.db");
    open_db("db/test_insert_seq.db");

    char buf[200];
    
    printf("Insert sequence start(%d)\n", INSERT_NUM);

    int start = time(0);
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", i);
        insert(i, buf);
    }
    int end = time(0); 
    printf("insert time: %d\n", end - start);

    int success = 1;
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", i);
        char * record = find(i);
        if(record == NULL || strcmp(record, buf) != 0) {
            success = 0;
            break;
        }
    }
    if(success) {
        puts("test insert sequence success");
    } else {
        puts("sequence validation fail");
    }

    delete_cache();
    return true;
}

int test_insert_random()
{
    unlink("db/test_insert_random.db");
    open_db("db/test_insert_random.db");

    char buf[200];
    int indices[INSERT_NUM];

    srand((unsigned int)time(0));
    printf("Insert random start(%d)\n", INSERT_NUM);

    int start = time(0);
    for(int i=0; i<INSERT_NUM; ++i) {
        indices[i] = (int)(rand() % 0x7FFFFF);
        sprintf(buf, "I'm a record %d", indices[i]);
        insert(indices[i], buf);
    }
    int end = time(0);
    printf("insert time: %d\n", end - start);

    int success = 1;
    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", indices[i]);
        char * record = find(indices[i]);
        
        if(record == NULL || strcmp(record, buf) != 0) {
            success = 0;
            break;
        } 
    }
    if(success) {
        puts("test insert random success");
    } else {
        puts("random validation failed");
    }
    
    delete_cache();
    return true;
}
