#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {

    if (argc < 2) {
        printf("syntax: %s <*.db file>\n", argv[0]);
        return 0;
    }

    init_db(100000);
    int fd = open_table(argv[1]);

    char instruction;
    char value_buf[1000];
    llu input, range2;
    int verbose_output = 0;

    // printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%llu", &input);
            delete(fd, input);
            print_tree(fd);
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
                printf("Couldn't find\n");
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
            scanf("%s", value_buf);
            if(fd) {
                close_table(fd);
            }
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
