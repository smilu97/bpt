#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {

    if (argc < 3) {
        printf("syntax: %s <*.db file> <buffer num>\n", argv[0]);
        return 0;
    }
    
    int buf_num = atoi(argv[2]);
    if(buf_num == 0) {
        printf("Failed to get the number of buffer\n");
        return 0;
    }

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    init_db(buf_num);
    int fd = open_table(argv[1]);
    int fd2;

    char instruction;
    char value_buf[1000];
    char buf2[1000];
    llu input, range2;
    int verbose_output = 0;

    // printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'j':
            scanf("%s%s", value_buf, buf2);
            fd2 = open_table(value_buf);
            join_table(fd, fd2, buf2);
            close_table(fd2);
            break;
        case 'd':
            scanf("%llu", &input);
            delete(fd, input);
            if(verbose_output) print_tree(fd);
            break;
        case 'i':
            scanf("%llu", &input);
            scanf("%s", value_buf);
            insert(fd, input, value_buf);
            if(verbose_output) print_tree(fd);
            break;
        case 'f':
        case 'p':
            scanf("%llu", &input);
            char * res = find(fd, input);
            if(res) {
                printf("Key: %llu, Value: %s\n", input, res);
            } else {
                printf("Not Exists\n");
            }
            break;
        case 'r':
            scanf("%llu %llu", &input, &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            find_and_print_range(fd, input, range2);
            break;
        case 'l':
            print_all(fd);
            break;
        case 'o':
            scanf("%d", &buf_num);
            scanf("%s", value_buf);
            shutdown_db();
            init_db(buf_num);
            fd = open_table(value_buf);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            close_table(fd);
            shutdown_db();
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(fd);
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        case 'x':
            destroy_tree(fd);
            print_tree(fd);
            break;
        default:
            // usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        // printf("> ");
    }
    printf("\n");
    close_table(fd);
    shutdown_db();

    return EXIT_SUCCESS;
}
