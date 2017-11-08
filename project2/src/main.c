#include "dbpt.h"

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
    int input;

    // printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%d", &input);
            delete(fd, input);
            print_tree();
            break;
        case 'i':
            scanf("%d", &input);
            scanf("%s", value_buf);
            insert(fd, input, value_buf);
            // print_tree();
            break;
        case 'f':
        case 'p':
            scanf("%d", &input);
            char * res = find(fd, input);
            if(res) {
                printf("Key: %d, Value: %s\n", input, res);
            } else {
                printf("Couldn't find\n");
            }
            break;
        case 'r':
            // scanf("%d %d", &input, &range2);
            // if (input > range2) {
            //     int tmp = range2;
            //     range2 = input;
            //     input = tmp;
            // }
            // find_and_print_range(root, input, range2, instruction == 'p');
            break;
        case 'l':
            // print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            close_table(fd);
            shutdown_db();
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree();
            break;
        case 'v':
            // verbose_output = !verbose_output;
            break;
        case 'x':
            // if (root)
            //     root = destroy_tree(root);
            print_tree();
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
