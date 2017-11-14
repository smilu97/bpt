
/* Author: smilu97
 */

 #include "bpt.h"

 int main(int argc, char** argv)
 {
     int fd;
     init_db(10000);
     if(argc < 2) {
         printf("usage: %s <*.db> <page_num>\n", argv[0]);
         return 0;
     }

     if(argc > 1) {
         if((fd = open_table(argv[1])) == 0) {
             printf("Failed to open file!\n");
             return -1;
         }
     }

     if(argc > 2) {
        MemoryPage * mem = get_page(fd, atoi(argv[2]));
        if(((InternalPage*)(mem->p_page))->header.isLeaf) {
            describe_leaf(mem);
        } else {
            describe_internal(mem);
        }
     } else {
         MemoryPage * m_head = get_header_page(fd);
         describe_header((HeaderPage*)m_head->p_page);
     }
     shutdown_db();

     return 0;
 }