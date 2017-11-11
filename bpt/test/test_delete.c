
/*
 * Author: smilu97
 */

#include "tests.h"

#define ITEM_LIMIT 10000

const char * test_value_format = "I'm record %d";

int test_delete()
{
    unlink("db/test_delete.db");

    init_db(10000);
    int fd = open_table("db/test_delete.db");

    char buf[100];

    for(int i=0; i<ITEM_LIMIT; ++i) {
        sprintf(buf, test_value_format, i);
        if(insert(fd, i, buf)) {
            myerror("Failed to insert in test_delete");
            return false;
        }
    }
    
    for(int i=0; i<ITEM_LIMIT; i += 2) {
        if(delete(fd, i)) {
            myerror("Failed to delete in test_delete");
            return false;
        }
    }

    for(int i=0; i<ITEM_LIMIT; i += 2) {
        sprintf(buf, test_value_format, i);
        char * res = find(fd, i);
        if(res != NULL) {
            myerror("Maybe failed to delete");
            return false;
        }
    }

    for(int i=1; i<ITEM_LIMIT; i += 2) {
        sprintf(buf, test_value_format, i);
        char * res = find(fd, i);
        if(res == 0 || strcmp(res, buf) != 0) {
            myerror("Failed to find");
            printf("found: %s\n", res);
            printf("made : %s\n", buf);
            return false;
        }
    }

    puts("test delete success");

    close_table(fd);
    shutdown_db();
    return true;
}

int test_delete_2()
{
    unlink("db/test_delete_2.db");

    init_db(10000);
    int fd = open_table("db/test_delete_2.db");

    char buf[100];

    for(int i=0; i<ITEM_LIMIT; ++i) {
        sprintf(buf, test_value_format, i);
        if(insert(fd, i, buf)) {
            myerror("Failed to insert in test_delete");
            return false;
        }
    }
    
    for(int i=0; i<ITEM_LIMIT; ++i) {
        if(delete(fd, i)) {
            myerror("Failed to delete in test_delete");
            return false;
        }
    }

    printf("test delete_2 success\n");
    close_table(fd);
    shutdown_db();
    return true;
}
