
/*
 * Author: smilu97
 */

#include "tests.h"

#define ITEM_LIMIT 1000

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
            perror("Failed to insert in test_delete");
            return false;
        }
    }
    
    for(int i=0; i<ITEM_LIMIT; i += 2) {
        if(delete(fd, i)) {
            perror("Failed to delete in test_delete");
            return false;
        }
    }

    for(int i=0; i<ITEM_LIMIT; i += 2) {
        sprintf(buf, test_value_format, i);
        char * res = find(fd, i);
        if(res != NULL) {
            perror("Maybe failed to delete");
            return false;
        }
    }

    for(int i=1; i<ITEM_LIMIT; i += 2) {
        sprintf(buf, test_value_format, i);
        char * res = find(fd, i);
        if(strcmp(res, buf) != 0) {
            perror("Failed to find");
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
