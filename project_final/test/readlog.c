
#include "transaction.h"

int main(int argc, char ** argv)
{
    if(argc < 2) {
        printf("syntax: %s <logfile>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDWR, 0644);
    if(fd == -1) {
        printf("Failed to open logfile\n");
        return -1;
    }

    LogUnit unit;
    int sz;
    while( (sz = read(fd, &unit, sizeof(LogUnit))) == sizeof(LogUnit) ) {
        char * desc = logunit_tostring(&unit);
        printf("%s\n", desc);
        free(desc);
    }

    close(fd);

    return 0;
}
