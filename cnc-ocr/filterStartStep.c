
#include "Common.h"

bool scheduleNextFilter(u32 i, u32 j, candidatesInfoItem info, cncHandle_t cHandle, cncHandle_t fHandle, Context *context) {
    // Continue
    if (info.item->count > info.item->confirmedCount) {
        // Move to next batch of factors
        // request next (demand-driven)
        cncPutChecked_factorsReq(CNC_NULL_HANDLE, j, false, context);
        DEBUG_LOG("Requested factor batch %s\n", nextFactorReqTag);
        // schedule next filter
        cncPut_candidatesInfo(info.handle, i, j, context);
        cncPut_candidates(cHandle, i, j, context);
        cncPrescribe_filterContinueStep(i, j, context);
        return 1;
    }
    // Done
    else if (info.item->summarizeFlag == KEEP_NONE_FLAG) {
        DEBUG_LOG("Put batch %d summarized\n", i);
        cncPut_primesInfo(info.handle, i, context);
        cncPut_primes(CNC_NULL_HANDLE, i, context);
        CNC_DESTROY_ITEM(cHandle); // release full batch's memory
        return 0;
    }
    else {
        if (info.item->summarizeFlag == KEEP_FACTOR_FLAG) {
            info.item->confirmedCount = 0; // summarized; count == confirmedCount implies full
        }
        DEBUG_LOG("Put batch %d full\n", i);
        cncPut_primesInfo(info.handle, i, context);
        cncPut_primes(cHandle, i, context);
        return 0;
    }
}

void filterStartStep(u32 i, u32 keep, u32 base, u32 count, factorsItem factors0, Context *context) {
    // Filter first batch, using the entire range starting from `base'
    uIntPrime end = base + CANDIDATE_BATCH_COUNT;
    PrimeFactor *factors = factors0.item;
    // Fix base so it's not a multiple of 2 or 3
    base |= 1; // add 1 if even
    if (base % 3 == 0) base += 2; // skip multiples of 3
    // Set up memory for storing prime candidate that pass this filter
    uIntPrime *candidates;
    cncHandle_t candidatesHandle = cncCreateItem_candidates(&candidates, CANDIDATE_BATCH_COUNT);
    candidatesInfoItem info;
    info.handle = cncCreateItem_candidatesInfo(&info.item, 1);
    info.item->baseValue = base;
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
    info.item->count = checkedCount;
    info.item->confirmedCount = confirmedCount;
    info.item->summarizeFlag = keep;
    scheduleNextFilter(i, 1, info, candidatesHandle, factors0.handle, context);
}


