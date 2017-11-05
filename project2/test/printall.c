
/* Author: smilu97
 */


#include "dbpt.h"

int main(int argc, char** argv)
{
    if(argc < 2) {
        printf("syntax: %s <*.db file>", argv[0]);
        return 0;
    }

    open_db(argv[1]);
    puts("opened file");

    print_all();

    return 0;
}