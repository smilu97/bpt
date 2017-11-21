
#include "tests.h"

#define N 1000000

int arr[N];

int test_perf(int buf_num)
{
    struct timespec begin, end;
    double time;

    char buf[255];
    char * res;

    init_db(buf_num);

    unlink("db/test_perf_seq.db");
    int fd = open_table("db/test_perf_seq.db");
    // Sequence
    clock_gettime(CLOCK_MONOTONIC, &begin);
    for(int i=0; i<N; ++i) {
        sprintf(buf, "I'm a record %d\n", i);
        if(insert(fd, i, buf)) {
            printf("Failed in sequence insert %d\n", i);
            shutdown_db();
            return -1;
        }
    }
    for(int i=0; i<N; ++i) {
        sprintf(buf, "I'm a record %d\n", i);
        if((res = find(fd, i)) == 0 || strcmp(res, buf) != 0) {
            printf("Failed to sequence find %d\n", i);
            shutdown_db();
            return -1;
        }
    }
    for(int i=0; i<N; ++i) {
        if(delete(fd, i)) {
            printf("Failed to sequence delete %d\n", i);
            shutdown_db();
            return -1;
        }
    }
    close_table(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0f;

    printf("Sequence: %f\n", time);

    // Random
    for(int i=0; i<N; ++i) arr[i] = i;
    shuffle_d(arr, N);
    unlink("db/test_perf_random.db");
    fd = open_table("db/test_perf_random.db");

    clock_gettime(CLOCK_MONOTONIC, &begin);
    for(int i=0; i<N; ++i) {
        sprintf(buf, "I'm a record %d,%d\n", i, arr[i]);
        if(insert(fd, arr[i], buf)) {
            printf("Failed in random insert %d,%d\n", i, arr[i]);
            shutdown_db();
            return -1;
        }
    }
    for(int i=0; i<N; ++i) {
        sprintf(buf, "I'm a record %d,%d\n", i, arr[i]);
        if((res = find(fd, arr[i])) == 0 || strcmp(res, buf) != 0) {
            printf("Failed to random find %d,%d\n", i, arr[i]);
            shutdown_db();
            return -1;
        }
    }
    for(int i=0; i<N; ++i) {
        if(delete(fd, arr[i])) {
            printf("Failed to random delete %d,%d\n", i, arr[i]);
            shutdown_db();
            return -1;
        }
    }
    close_table(fd);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec) / 1000000000.0f;

    printf("Random: %f\n", time);

    shutdown_db();

    return 0;
}
