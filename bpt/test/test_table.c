
#include "tests.h"

#define INSERT_NUM 100

int test_table()
{
    init_db(1000);

    char pathbuf[200];
    char valuebuf[200];
    
    for(int idx = 0; idx < 10; ++idx) {
        sprintf(pathbuf, "db/test_table_%d.db", idx + 1);
        int fd = open_table(pathbuf);

        for(int i = INSERT_NUM * idx; i < INSERT_NUM * (idx + 1); ++i) {
            sprintf(valuebuf, "I'm a record %d", i);
            insert(fd, i, valuebuf);
        }

        close_table(fd);
    }

    int success = 1;
    for(int idx = 0; idx < 10; ++idx) {
        sprintf(pathbuf, "db/test_table_%d.db", idx + 1);
        int fd = open_table(pathbuf);

        for(int i = INSERT_NUM * idx; i < INSERT_NUM * (idx + 1); ++i) {
            sprintf(valuebuf, "I'm a record %d", i);
            char * result = find(fd, i);
            if(strcmp(valuebuf, result) != 0) {
                success = 0;
                break;
            }
        }

        close_table(fd);

        if(success == 0) break;
    }

    shutdown_db();
    
    if(success) {
        puts("Success in test table");
        return 1;
    } else {
        myerror("Fail in test table");
        return 0;
    }
}
