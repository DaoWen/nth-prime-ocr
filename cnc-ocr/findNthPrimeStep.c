
#include "Common.h"
void findNthPrimeStep(u32 N, u32 i, primesItem primes, Context *context){
	DEBUG_LOG("In batch %d\n", i);
    uIntPrime nthPrime = primes.item[N-1]; // -1 for 0-based index
	putNthPrime(N, nthPrime, context);
}

