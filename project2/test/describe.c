
/* Author: smilu97
 */

 #include "dbpt.h"

 int main(int argc, char** argv)
 {
     if(argc < 2) {
         printf("usage: %s <*.db> <page_num>\n", argv[0]);
         return 0;
     }

     if(argc > 1) {
         if(open_db(argv[1])) {
             printf("Failed to open file!\n");
             return -1;
         }
     }

     if(argc > 2) {
        MemoryPage * mem = get_page(atoi(argv[2]));
        if(((InternalPage*)(mem->p_page))->header.isLeaf) {
            describe_leaf(mem);
        } else {
            describe_internal(mem);
        }
     } else {
         describe_header(&headerPage);
     }
 }