#include "Primes.h"

/**
 * Collect prime numbers into batches of prime factors
 *
 * Parameters:
 *  - batchOffset: index of next prime to collect in the current input batch
 *  - factorCount: number of factors collected into the current batch
 *
 * Dependences:
 *  - inputBatch: batch of prime numbers to collect into prime factors batch
 *  - collectedBatch: current (partial) batch of prime factors collected thus far
 *    This can be null, in which case a new batch is allocated
 *  - factorsReq: The request for this block of factors (no data--just keeps it demand-driven)
 *
 */
void Primes_collectFactorsStep(cncTag_t i, cncTag_t j, cncTag_t batchOffset, cncTag_t factorCount, PrimeFactor *collected, CandidatesInfo *primesInfo, uIntPrime *primes, void *factorsReq, PrimesCtx *ctx) {
    PrimeFactor *collectedBatch = collected;
    assert(primes && "Should never require factors past requested primes.");
    bool hitSummarizedBatch = !IS_FACTOR_BATCH(primesInfo);
    // Make sure we have memory for collecting the factors
    if (!collectedBatch) {
        DEBUG_LOG("Starting factor batch %u\n", i);
        collectedBatch = cncItemCreateVector_factors(FACTOR_BATCH_COUNT);
        assert(factorCount == 0 && "Should be starting a new batch of factors");
    }
    // Process next batch
    if (hitSummarizedBatch) {
        // make last factor really big so that it's always bigger than sqrt of max prime
        collectedBatch[factorCount++] = PRIME_FACTOR_OF(UINTP_MAX_ROOT);
    }
    else {
        while (batchOffset<primesInfo->count && factorCount<FACTOR_BATCH_COUNT) {
            uIntPrime p = primes[batchOffset++];
            collectedBatch[factorCount++] = PRIME_FACTOR_OF(p);
        }
    }
    assert(factorCount<=FACTOR_BATCH_COUNT && "Overfull factor batch");
    // TODO - what if both are done/empty at the same time? does that work correctly?
    // Factors full
    if (hitSummarizedBatch || factorCount>=FACTOR_BATCH_COUNT) {
        assert(collectedBatch[0].prime != 0 && "Writing back garbage");
        // output filled batch
        cncPut_factors(collectedBatch, i, ctx);
        // recur, continuing work on current input batch (new factor batch)
        cncPrescribe_collectFactorsStep(i+1, j, batchOffset, 0, ctx);
        // output empty (new) factor batch for next
        cncPut_collected(NULL, i+1, j, ctx);
        DEBUG_LOG("Created factor batch %u\n", i);
    }
    // Input primes batch exhausted
    else {
        assert(batchOffset==primesInfo->count && "Dropped input primes.");
        // recur, continuing to collect in current factor batch (new input batch)
        cncPrescribe_collectFactorsStep(i, j+1, 0, factorCount, ctx);
        // output current factor batch for next
        cncPut_collected(collectedBatch, i, j+1, ctx);
        DEBUG_LOG("Collecting factors from primes batch %u\n", j);
    }
}
