#include "Primes.h"

u32 scheduleNextFilter(cncTag_t i, cncTag_t j, CandidatesInfo *info, uIntPrime *candidates, PrimeFactor *factors, PrimesCtx *ctx) {
    // Continue
    if (info->count > info->confirmedCount) {
        // Move to next batch of factors
        // request next (demand-driven)
        cncPutChecked_factorsReq(NULL, j, false, ctx);
        DEBUG_LOG("Requested factor batch %s\n", nextFactorReqTag);
        // schedule next filter
        cncPut_candidatesInfo(info, i, j, ctx);
        cncPut_candidates(candidates, i, j, ctx);
        cncPrescribe_filterContinueStep(i, j, ctx);
        return 1;
    }
    // Done
    else if (info->summarizeFlag == KEEP_NONE_FLAG) {
        DEBUG_LOG("Put batch %ld summarized\n", i);
        cncPut_primesInfo(info, i, ctx);
        cncPut_primes(NULL, i, ctx);
        cncItemDestroy(candidates); // release full batch's memory
        return 0;
    }
    else {
        if (info->summarizeFlag == KEEP_FACTOR_FLAG) {
            info->confirmedCount = 0; // summarized; count == confirmedCount implies full
        }
        DEBUG_LOG("Put batch %ld full\n", i);
        cncPut_primesInfo(info, i, ctx);
        cncPut_primes(candidates, i, ctx);
        return 0;
    }
}

void Primes_filterStartStep(cncTag_t i, cncTag_t keep, cncTag_t base, cncTag_t count, PrimeFactor *factors, PrimesCtx *ctx) {
    // Filter first batch, using the entire range starting from `base'
    uIntPrime end = base + CANDIDATE_BATCH_COUNT;
    // Fix base so it's not a multiple of 2 or 3
    base |= 1; // add 1 if even
    if (base % 3 == 0) base += 2; // skip multiples of 3
    // Set up memory for storing prime candidate that pass this filter
    uIntPrime *candidates = cncItemCreateVector_candidates(CANDIDATE_BATCH_COUNT);
    CandidatesInfo *info = cncItemCreate_candidatesInfo();
    info->baseValue = base;
    // Start with inc=4 if base%3 is 1, or inc=2 if base%3=2
    uIntPrime startInc = 6-(base%3)*2;
    assert((base+startInc)%3 > 0 && "Increment should skip all multiples of 3");
    // Filter all candidate by the given batch of prime factor
    // Skip primes and multiples of 3 by alternating inc between 2 and 4
    u32 checkedCount = 0, confirmedCount = 0;
    for (uIntPrime n=base, inc=startInc; n<end; n+=inc, inc=6-inc) {
        bool belowBound = false;
        uIntPrime factorBound = ZSQRT(n); // Only need to check factor thru sqrt(n)
        for (u32 i=0; i<FACTOR_BATCH_COUNT && (belowBound = factors[i].prime<factorBound); i++) {
            // If you find a prime factor then n isn't prime
            if (DIVIDES(factors[i], n)) goto lbl_next_candidate;
        }
        // Completely confirmed that n is prime (checked all possible factors)
        if (!belowBound) confirmedCount++;
        // No prime factor, so this candidate passes
        candidates[checkedCount++] = n;
lbl_next_candidate:; // labeled continue (semi-colon needed to avoid empty block)
    }
    // Results out
    info->count = checkedCount;
    info->confirmedCount = confirmedCount;
    info->summarizeFlag = keep;
    scheduleNextFilter(i, 1, info, candidates, factors, ctx);
}

