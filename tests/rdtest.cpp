// std::random_device through gfamiga_random.c (/dev/urandom wrapped): two
// reads that differ. Copyright (c) 2026 AmiCradle Team
#include <random>
#include <cstdio>
int main(){ std::random_device rd; unsigned a=rd(), b=rd(); std::printf("%08x %08x\n",a,b); return a==b; }
