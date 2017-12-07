
/*
*  Author: smilu97, Gim Yeongjin
*/

#ifndef __HASH_H__
#define __HASH_H__

typedef unsigned long long llu;
typedef signed   long long lld;

/*
*  64 bit Mix Functions
*  Copied from https://gist.github.com/badboy/6267743
*/
llu hash_llu(llu key, llu mod);

#endif
