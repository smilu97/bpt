
/*
 * Author: smilu97
 */

#include "tests.h"

int main(int argc, char ** argv)
{
    if(argc >= 3 && strcmp(argv[1], "perf") == 0) {
        test_perf(atoi(argv[2]));
        return 0;
    }
    if(
        test_page() &&
        test_insert_sequence() &&
        test_insert_random() &&
        test_delete_2() &&
        test_delete() &&
        test_delete_random() &&
        test_table()
    ) {
        puts("SUCCESS ^ ^");
    }
    else {
        puts("FAIL - -");
    }
    
    return 0;
}
