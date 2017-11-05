
/*
 * Author: smilu97
 */

#include "tests.h"

int found[1000];
int found_idx = 0;
int fail[1000];
int fail_idx = 0;

#define INSERT_NUM (100000)

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
    printf("time: %d\n", end - start);

    puts("test insert sequence success");
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

    for(int i=0; i<INSERT_NUM; ++i) {
        indices[i] = (int)(rand() % 0x7FFFFF);
        sprintf(buf, "I'm a record %d", indices[i]);
        insert(indices[i], buf);
    }

    found_idx = 0;
    fail_idx = 0;

    for(int i=0; i<INSERT_NUM; ++i) {
        sprintf(buf, "I'm a record %d", indices[i]);
        char * record = find(indices[i]);
        
        if(record == NULL) {
            return false;
        } else {
            if(strcmp(buf, record) != 0) {
                printf("fail in strcmp\n");
                return false;
            }
        }
    }

    puts("test insert random success");
    delete_cache();
    return true;
}
