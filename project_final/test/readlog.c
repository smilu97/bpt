
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

    LogRecord unit;
    int sz;
    lseek(fd, 0, SEEK_SET);
    while( (sz = read(fd, &unit, LOGSIZE_BEGIN)) == LOGSIZE_BEGIN ) {
        if(unit.type == LT_UPDATE) {
            sz = read(fd, ((char*)&unit) + LOGSIZE_BEGIN, LOGSIZE_UPDATE - LOGSIZE_BEGIN);
        }
        char * desc = logrecord_tostring(&unit);
        printf("%s\n", desc);
        free(desc);

        lseek(fd, unit.lsn, SEEK_SET);
    }

    close(fd);

    return 0;
}
