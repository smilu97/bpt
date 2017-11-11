
/*
 * Author: smilu97
 */

#include "tests.h"

int main(int argc, char ** argv)
{
    if(test_page() && test_insert_sequence() && test_insert_random() && test_delete_2() && test_table()
        && test_delete()) {
        puts("SUCCESS ^ ^");
    } else {
        puts("FAIL - -");
    }
    
    return 0;
}
