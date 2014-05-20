
#include "Common.h"

extern bool scheduleNextFilter(u32 i, u32 j, candidatesInfoItem info, cncHandle_t cHandle, cncHandle_t fHandle, Context *context);

void filterContinueStep( u32 i, u32 j, factorsItem factors0, candidatesInfoItem info, candidatesItem candidates0, Context *context){
    uIntPrime *candidates = candidates0.item;
    PrimeFactor *factors = factors0.item;
    // Filter all candidate by the given batch of prime factor
    // Skip primes and multiples of 3 by alternating inc between 2 and 4
    u32 confirmedCount = info.item->confirmedCount;
    u32 checkedCount = confirmedCount;
    for (u32 j=info.item->confirmedCount; j<info.item->count; j++) {
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
    info.item->count = checkedCount;
    info.item->confirmedCount = confirmedCount;
    scheduleNextFilter(i, j+1, info, candidates0.handle, factors0.handle, context);
}


