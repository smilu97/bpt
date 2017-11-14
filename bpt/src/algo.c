
#include "algo.h"

int min_int(int a, int b)
{
    if(a < b) return a;
    return b;
}

int max_int(int a, int b)
{
    if(a > b) return a;
    return b;
}

long long max_lld(long long a, long long b)
{
    if(a > b) return a;
    return b;
}

long long min_lld(long long a, long long b)
{
    if(a < b) return a;
    return b;
}

void shuffle_d(int * arr, int len)
{
    for(int i=0; i<len; ++i) {
        int j = rand() % len;
        
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

void myerror(const char * str)
{
    fprintf(stderr, ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, str);
}
