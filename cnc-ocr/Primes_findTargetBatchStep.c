#include "Primes.h"

/**
 * Step function defintion for "findTargetBatchStep"
 */
void findTargetBatchStep(cncTag_t n, ReducedResult *reducedPrimes, PrimesCtx *ctx) {
    s64 N = n;
    N -= FACTOR_BATCH_COUNT+2;    // offset N by the seeded prime count
    assert(N < reducedPrimes->count && "Didn't find enough primes");
    N -= reducedPrimes->offset; // offset by all the summarized prime results
    for (u32 i=0; i<reducedPrimes->batchCount; i++) {
        BatchRef batch = reducedPrimes->batches[i];
        N -= batch.count;
        if (N <= 0) {
            N += batch.count;
            // schedule EDT to put nth prime with the found batch's guid
            cncPrescribe_findNthPrimeStep(N, batch.index, ctx);
            return;
        }
    }
    CNC_REQUIRE(0, "Couldn't find Nth prime.\n");
}
