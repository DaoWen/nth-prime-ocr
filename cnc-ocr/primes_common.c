#include "Primes.h"

// Calculating upper bound for the nth prime number
// See http://stackoverflow.com/a/1069023/1427124
uIntPrime primeUpperBound(uPrimeCount x) {
    double boundTerm = (x >= 7022) ? 0.9385 : 0.0;
    double n = x;
    return (uIntPrime) ceil(n * log(n) + n * (log(log(n)) - boundTerm));
}

// Calculating lower bound for the nth prime number
// See http://en.wikipedia.org/wiki/Prime_number_theorem#Approximations%5Ffor%5Fthe%5Fnth%5Fprime%5Fnumber
uIntPrime primeLowerBound(uPrimeCount x) {
    double n = x;
    return (uIntPrime) floor(n * (log(n) + log(log(n)) - 1));
}

// Upper bound on the prime counting function (pi)
// See http://en.wikipedia.org/wiki/Prime_number_theorem#Bounds_on_the_prime-counting_function
// and http://www.jstor.org/discover/10.2307/2371291
uPrimeCount piUpperBound(uIntPrime x) {
    assert(55 <= x && "x is too small to estimate pi(x)");
    double n = x;
    return (uPrimeCount) floor(n / (log(n) - 4));
}

/**
 * Computes the first FACTOR_BATCH_COUNT primes starting at 5.
 * This means the last prime returned is the (FACTOR_BATCH_COUNT+2)th prime.
 */
PrimeFactor *primeSeeds() {
    PrimeFactor *factors = cncItemCreateVector_factors(FACTOR_BATCH_COUNT);
    s32 foundCount = 0;
    // Skip evens and multiples of 3 by alternating inc between 2 and 4
    for (uIntPrime n=5, inc=2; foundCount<FACTOR_BATCH_COUNT; n+=inc, inc=6-inc) {
        for (s32 i=0; i<foundCount; i++) {
            // If you find a prime factor then n isn't prime
            if (DIVIDES(factors[i], n)) goto lbl_next_candidate;
        }
        // No prime factor, so we found a new prime
        factors[foundCount] = PRIME_FACTOR_OF(n); 
        foundCount++;
lbl_next_candidate:; // labeled continue (semi-colon needed to avoid empty block)
    }
    // Results out
    assert(foundCount == FACTOR_BATCH_COUNT);
    return factors;
}

void putNthPrime(uPrimeCount n, uIntPrime nthPrime, PrimesCtx *ctx) {
    uIntPrime *nthPrimePtr = cncItemCreate_nthPrime();
    *nthPrimePtr = nthPrime;
    cncPut_nthPrime(nthPrimePtr, ctx);
}

