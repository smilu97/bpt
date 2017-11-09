
#ifndef __ALGO_H__
#define __ALGO_H__

#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

#define ANSI_COLOR_RESET   "\x1b[0m"

int min_int(int a, int b);
int max_int(int a, int b);

long long min_lld(long long a, long long b);
long long max_lld(long long a, long long b);

void myerror(const char * str);

#endif
