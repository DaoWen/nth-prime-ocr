#include "Primes.h"

extern u32 scheduleNextFilter(cncTag_t i, cncTag_t j, CandidatesInfo *info, uIntPrime *candidates, PrimeFactor *factors, PrimesCtx *ctx);

/**
 * Step function defintion for "filterContinueStep"
 */
void filterContinueStep(cncTag_t i, cncTag_t j, PrimeFactor *factors, CandidatesInfo *info, uIntPrime *candidates, PrimesCtx *ctx) {
    // Filter all candidate by the given batch of prime factor
    // Skip primes and multiples of 3 by alternating inc between 2 and 4
    u32 confirmedCount = info->confirmedCount;
    u32 checkedCount = confirmedCount;
    for (u32 j=info->confirmedCount; j<info->count; j++) {
        bool belowBound = false;
        uIntPrime n = candidates[j];
        uIntPrime factorBound = ZSQRT(n); // Only need to check factor thru sqrt(n)
        for (u32 i=0; i<FACTOR_BATCH_COUNT && (belowBound = factors[i].prime<factorBound); i++) {
            // If you find a prime factor then n isn't prime
            if (DIVIDES(factors[i], n)) goto lbl_next_candidate;
        }
        // No prime factor, so this candidate passes
        candidates[checkedCount++] = n;
        // Completely confirmed that n is prime (checked all possible factors)
        if (!belowBound) confirmedCount++;
lbl_next_candidate:; // labeled continue (semi-colon needed to avoid empty block)
    }
    // Results out
    info->count = checkedCount;
    info->confirmedCount = confirmedCount;
    scheduleNextFilter(i, j+1, info, candidates, factors, ctx);
}

