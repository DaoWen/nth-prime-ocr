
#include "Common.h"

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
void collectFactorsStep(u32 i, u32 j, u32 batchOffset, u32 factorCount, collectedItem collected,
        primesInfoItem primesInfo, primesItem primes, factorsReqItem factorsReq, Context *context) {
    cncHandle_t collectedBatchHandle = collected.handle;
    PrimeFactor *collectedBatch = collected.item;
    assert(primes.item && "Should never require factors past requested primes.");
    bool hitSummarizedBatch = !IS_FACTOR_BATCH(primesInfo.item);
    // Make sure we have memory for collecting the factors
    if (!collectedBatch) {
        DEBUG_LOG("Starting factor batch %u\n", i);
        collectedBatchHandle = cncCreateItem_factors(&collectedBatch, FACTOR_BATCH_COUNT);
        assert(factorCount == 0 && "Should be starting a new batch of factors");
    }
    // Process next batch
    if (hitSummarizedBatch) {
        // make last factor really big so that it's always bigger than sqrt of max prime
        collectedBatch[factorCount++] = PRIME_FACTOR_OF(UINTP_MAX_ROOT);
    }
    else {
        while (batchOffset<primesInfo.item->count && factorCount<FACTOR_BATCH_COUNT) {
            uIntPrime p = primes.item[batchOffset++];
            collectedBatch[factorCount++] = PRIME_FACTOR_OF(p);
        }
    }
    assert(factorCount<=FACTOR_BATCH_COUNT && "Overfull factor batch");
    // TODO - what if both are done/empty at the same time? does that work correctly?
    // Factors full
    if (hitSummarizedBatch || factorCount>=FACTOR_BATCH_COUNT) {
        assert(collectedBatch[0].prime != 0 && "Writing back garbage");
        // output filled batch
        cncPut_factors(collectedBatchHandle, i, context);
        // recur, continuing work on current input batch (new factor batch)
        cncPrescribe_collectFactorsStep(i+1, j, batchOffset, 0, context);
        // output empty (new) factor batch for next
        cncPut_collected(CNC_NULL_HANDLE, i+1, j, context);
        DEBUG_LOG("Created factor batch %u\n", i);
    }
    // Input primes batch exhausted
    else {
        assert(batchOffset==primesInfo.item->count && "Dropped input primes.");
        // recur, continuing to collect in current factor batch (new input batch)
        cncPrescribe_collectFactorsStep(i, j+1, 0, factorCount, context);
        // output current factor batch for next
        cncPut_collected(collectedBatchHandle, i, j+1, context);
        DEBUG_LOG("Collecting factors from primes batch %u\n", j);
    }
}


