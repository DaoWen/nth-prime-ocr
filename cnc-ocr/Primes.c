#include "Primes.h"


/**
 * Check if two ranges [a0, a1) and [b0, b1) overlap
 */
bool rangesOverlap(uIntPrime a0, uIntPrime a1, uIntPrime b0, uIntPrime b1) {
    return (a0 <= b0) ? (b0 < a1) : rangesOverlap(b0, b1, a0, a1);
}

void Primes_cncInitialize(PrimesArgs *args, PrimesCtx *ctx) {
    
    uPrimeCount N = ctx->n;
    // Trivial case: 1st or 2nd prime
    if (N <= 2) {
        putNthPrime(N, (N == 1 ? 2 : 3), ctx);
    }
    else {
        // Generate seed primes
        PrimeFactor *seedBatch = primeSeeds();

        // Trivial case: prime was seeded
        if (N-2 <= FACTOR_BATCH_COUNT) {
            putNthPrime(N, seedBatch[N-3].prime, ctx);
        }
        // Non-trivial case: need to keep computing primes
        else {
            // Calculate bounds
            uIntPrime pnUpperBound = primeUpperBound(N)+1; // +1 to make it exclusive
            uIntPrime pnLowerBound = primeLowerBound(N);
            uIntPrime factorBound = ZSQRT(pnUpperBound)+1;
            PRINTF("UB: %u LB: %u FB: %u\n", pnUpperBound, pnLowerBound, factorBound);

            // Estimate how many segments are needed
            uIntPrime nextBase = LAST_FACTOR(seedBatch).prime+2;
            u32 primeBatchCount = (pnUpperBound - nextBase + CANDIDATE_BATCH_COUNT - 1) / CANDIDATE_BATCH_COUNT;
            PRINTF("Using %u batches\n", primeBatchCount);
            
            // Init factor batches with the seed batch
            cncPut_factors(seedBatch, 0, ctx);

            // Create batches
            for (u32 i=0; i<primeBatchCount; i++) {
                // Check if the resulting primes need to be kept around
                uIntPrime nextUBound = nextBase+CANDIDATE_BATCH_COUNT;
                bool inFactorRange = rangesOverlap(0, factorBound, nextBase, nextUBound);
                bool inTargetRange = rangesOverlap(nextBase, nextUBound, pnLowerBound, pnUpperBound*2);
                u32 factorFlag = inFactorRange ? KEEP_FACTOR_FLAG : 0;
                u32 targetFlag = inTargetRange ? KEEP_TARGET_FLAG : 0;
                u32 keep = factorFlag | targetFlag;
                u32 count = MIN(CANDIDATE_BATCH_COUNT, pnUpperBound-nextBase);
                // Start filter task
                cncPrescribe_filterStartStep(i, keep, nextBase, count, ctx);
                // next
                nextBase += CANDIDATE_BATCH_COUNT;
            }

            // Set up next batch of factors after seed
            cncPut_collected(NULL, 1, 0, ctx);
            cncPrescribe_collectFactorsStep(1, 0, 0, 0, ctx);

            // Combine results by folding
            s32 treeWidth  = primeBatchCount;
            s32 treeHeight = (u32) ceil(log(treeWidth)/log(2));
            for(s32 i = 0; i <treeWidth; i++) {
                //char *reducerTag = createTag(2, treeHeight-1, i);
                cncPrescribe_makeReducerLeafStep(treeWidth, treeHeight, i, ctx);
            }

            // Find the answer from folded/reduced results
            cncPrescribe_findTargetBatchStep(N, ctx);
        }
    }

    // Set finalizer function's tag
    Primes_await(ctx);

}


void Primes_cncFinalize(uIntPrime nthPrime, PrimesCtx *ctx) {
    PRINTF("The Nth prime where N=%lu is %lu.\n", (u64)ctx->n, (u64)nthPrime);
}

