#include "Primes.h"

/**
 * Step function defintion for "findNthPrimeStep"
 */
void findNthPrimeStep(cncTag_t n, cncTag_t i, uIntPrime *primes, PrimesCtx *ctx) {
	DEBUG_LOG("In batch %ld\n", i);
    uIntPrime nthPrime = primes[n-1]; // -1 for 0-based index
	putNthPrime(n, nthPrime, ctx);
}
