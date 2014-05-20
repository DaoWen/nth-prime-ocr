/*
 * Primes Generator for raw OCR
 */

#include "primes_common.h"

void initFactorBatchNexts(PrimeFactorBatch *batch) {
    ocrEventCreate(&batch->requestNextBatch, OCR_EVENT_IDEM_T, false);
    ocrEventCreate(&batch->nextBatch, OCR_EVENT_STICKY_T, true);
}

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
 * Check if two ranges [a0, a1) and [b0, b1) overlap
 */
bool rangesOverlap(uIntPrime a0, uIntPrime a1, uIntPrime b0, uIntPrime b1) {
    return (a0 <= b0) ? (b0 < a1) : rangesOverlap(b0, b1, a0, a1);
}

/**
 * Computes the first SEED_PRIMES_COUNT primes starting at 5.
 * This means the last prime returned is the (SEED_PRIMES_COUNT+2)th prime.
 */
ocrEdtDep_t primeSeeds() {
    ocrEdtDep_t factorDep;
    DBCREATE_BASIC(&factorDep.guid, (void**) &factorDep.ptr, SIZEOF_FACTOR_BATCH(SEED_PRIMES_COUNT));
    PrimeFactorBatch *factorBatch = factorDep.ptr;
    PrimeFactor *factors = factorBatch->entries;
    s32 foundCount = 0;
    // Skip primes and multiples of 3 by alternating inc between 2 and 4
    for (uIntPrime n=5, inc=2; foundCount<SEED_PRIMES_COUNT; n+=inc, inc=6-inc) {
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
    assert(foundCount == SEED_PRIMES_COUNT);
    factorBatch->count = foundCount;
    initFactorBatchNexts(factorBatch);
    return factorDep;
}

uPrimeCount parseNArg(u64 *packedArgs) {
    u64 argc = packedArgs[0];
    if (argc != 2) {
        PRINTF("%lu args, %lu\n", packedArgs[0], packedArgs[1]);
        ERROR_DIE("Expected argument N to calculate Nth prime.\n");
    }
    char *argv1 = ((char*)packedArgs)+packedArgs[2];
    s32 n = atoi(argv1);
    if (n <= 0) {
        ERROR_DIE("Value for N is not in range: %s\n", argv1);
    }
    return n;
}

ocrEdtDep_t filterCandidatesBase(uIntPrime base, PrimeFactorBatch *factorBatch) {
    uIntPrime end = base + CANDIDATE_BATCH_COUNT;
    PrimeFactor *factors = factorBatch->entries;
    // Fix base so it's not a multiple of 2 or 3
    base |= 1; // add 1 if even
    if (base % 3 == 0) base += 2; // skip multiples of 3
    // Set up memory for storing prime candidate that pass this filter
    ocrEdtDep_t batchDep;
    DBCREATE_BASIC(&batchDep.guid, (void**) &batchDep.ptr, SIZEOF_CANDIDATE_BATCH);
    CandidateBatch *candidateBatch = batchDep.ptr;
    candidateBatch->baseValue = base;
    SET_CBATCH_COOKIE(candidateBatch);
    uIntPrime *candidates = candidateBatch->entries;
    // Start with inc=4 if base%3 is 1, or inc=2 if base%3=2
    uIntPrime startInc = 6-(base%3)*2;
    assert((base+startInc)%3 > 0 && "Increment should skip all multiples of 3");
    // Filter all candidate by the given batch of prime factor
    // Skip primes and multiples of 3 by alternating inc between 2 and 4
    u32 fCount = factorBatch->count;
    u32 checkedCount = 0, confirmedCount = 0;
    for (uIntPrime n=base, inc=startInc; n<end; n+=inc, inc=6-inc) {
        bool belowBound = false;
        uIntPrime factorBound = ZSQRT(n); // Only need to check factor thru sqrt(n)
        for (u32 i=0; i<fCount && (belowBound = factors[i].prime<factorBound); i++) {
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
    CHECK_CBATCH_COOKIE(candidateBatch);
    candidateBatch->count = checkedCount;
    candidateBatch->confirmedCount = confirmedCount;
    return batchDep;
}

void filterCandidates(CandidateBatch *candidateBatch, PrimeFactorBatch *factorBatch) {
    uIntPrime *candidates = candidateBatch->entries;
    PrimeFactor *factors = factorBatch->entries;
    // Filter all candidate by the given batch of prime factor
    // Skip primes and multiples of 3 by alternating inc between 2 and 4
    s32 fCount = factorBatch->count;
    u32 confirmedCount = candidateBatch->confirmedCount;
    u32 checkedCount = confirmedCount;
    for (u32 j=candidateBatch->confirmedCount; j<candidateBatch->count; j++) {
        bool belowBound = false;
        uIntPrime n = candidates[j];
        uIntPrime factorBound = ZSQRT(n); // Only need to check factor thru sqrt(n)
        for (s32 i=0; i<fCount && (belowBound = factors[i].prime<factorBound); i++) {
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
    CHECK_CBATCH_COOKIE(candidateBatch);
    candidateBatch->count = checkedCount;
    candidateBatch->confirmedCount = confirmedCount;
}

ocrGuid_t reduceBatches(ReducedResult *lhs, ocrGuid_t lhsGuid, ReducedResult *rhs, ocrGuid_t rhsGuid) {
    ReducedResult *target;
    ocrGuid_t targetGuid;
    assert(lhs != rhs && "Aliased inputs");
    assert(lhs != NULL && rhs != NULL && "Shouldn't have null inputs to combine");
    assert((lhs->batchLimit == 0 || rhs->batchLimit > 0) && "Can't have full lhs and summarized rhs");
    uPrimeCount newBatchCount = lhs->batchCount + rhs->batchCount;
    // Case: both fit in rhs's allocated space
    if (rhs->batchLimit >= newBatchCount) {
        target = rhs;
        targetGuid = rhsGuid;
    }
    // Case: need to allocate a new block
    else {
        assert(rhs->batchLimit > 0);
        size_t newBatchLimit = rhs->batchLimit;
        while (newBatchLimit < newBatchCount) newBatchLimit *= 2;
        size_t newSize = sizeof(ReducedResult)+newBatchLimit*sizeof(BatchRef);
        DBCREATE_BASIC(&targetGuid, (void**) &target, newSize);
        target->batchLimit = newBatchLimit;
    }
    // copy rhs's entries (careful! target and rhs may be aliased!)
    memmove(target->batches+lhs->batchCount, rhs->batches, sizeof(BatchRef)*rhs->batchCount);
    // copy lhs's entries
    memcpy(target->batches, lhs->batches, sizeof(BatchRef)*lhs->batchCount);
    assert(target->batchLimit == 0 || !IS_NULL_GUID(target->batches[newBatchCount-1].guid));
    // update counts
    target->count = rhs->count + lhs->count;
    target->offset = rhs->offset + lhs->offset;
    target->batchCount = newBatchCount;
    // clean up
    assert(lhsGuid != targetGuid);
    ocrDbDestroy(lhsGuid); // free lhs memory
    if (rhsGuid != targetGuid) ocrDbDestroy(rhsGuid); // free rhs memory if not used
    return targetGuid;
}

/**
 * Converts result of filters into a reduction node
 */
ocrGuid_t reductionLeaf(CandidateBatch *batch, BatchRef *batchRef) {
    ocrGuid_t redGuid;
    ReducedResult *red;
    u32 batchLimit = IS_SUMMARY_BATCH(batch) ? 0 : 16;
    // TODO - technically we could have empty batches if we encounter huge gaps
    assert(batch->count > 0 && "Can't have empty batches!");
    DBCREATE_BASIC(&redGuid, (void**) &red, sizeof(ReducedResult) + batchLimit*sizeof(BatchRef));
    red->count = batch->count;
    if (batchLimit > 0) {
        red->offset = 0;
        red->batchCount = 1;
        red->batches[0] = *batchRef;
    }
    else {
        red->offset = batch->count;
        red->batchCount = 0;
    }
    red->batchLimit = batchLimit;
    return redGuid;
}

/* argument struct */
typedef struct {
    u32 batchOffset;
    ocrGuid_t outputPromise;
    ocrGuid_t recursiveTemplate;
} FactorCollectorParams;

/**
 * Collect prime numbers into batches of prime factors
 *
 * Parameters:
 *  - batchOffset: index of next prime to collect in the current input batch
 *  - outputPromiseGuid: guid for the event (promise) to put the next batch of factors
 *  - recursiveTemplate: EDT template guid for this function to make recursive calls easier
 *    (this way I only have to call ocrEdtTemplateCreate once)
 *
 * Dependences:
 *  - inputBatch: batch of prime numbers to collect into prime factors batch
 *  - collectedBatch: current (partial) batch of prime factors collected thus far
 *    This can be null, in which case a new batch is allocated
 *  - reqGuid: The request for this block of factors (no data--just keeps it demand-driven)
 *
 */
ocrGuid_t collectFactorsEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    FactorCollectorParams *args = (FactorCollectorParams*)paramv;
    CandidateBatch *inputBatch = depv[0].ptr;         // Input primes to collect into a factor batch
    ocrGuid_t inputBatchGuid = depv[0].guid;          // "
    PrimeFactorBatch *collectedBatch = depv[1].ptr;   // Partial batch of prime factors
    ocrGuid_t collectedBatchGuid = depv[1].guid;      // "
    assert(inputBatch && "Should never require factors past requested primes.");
    bool hitSummarizedBatch = !IS_FACTOR_BATCH(inputBatch);
    // Make sure we have memory for collecting the factors
    if (IS_NULL_GUID(collectedBatchGuid)) {
        DBCREATE_BASIC(&collectedBatchGuid, (void**) &collectedBatch, SIZEOF_FACTOR_BATCH(FACTOR_BATCH_COUNT));
        collectedBatch->count = 0;
        initFactorBatchNexts(collectedBatch);
    }
    // Process next batch
    u32 batchOffset = args->batchOffset;
    if (hitSummarizedBatch) {
        // make last factor really big so that it's always bigger than sqrt of max prime
        collectedBatch->entries[(collectedBatch->count)++] = PRIME_FACTOR_OF(UINTP_MAX_ROOT);
    }
    else {
        while (batchOffset<inputBatch->count && collectedBatch->count<FACTOR_BATCH_COUNT) {
            uIntPrime p = inputBatch->entries[batchOffset++];
            collectedBatch->entries[(collectedBatch->count)++] = PRIME_FACTOR_OF(p);
        }
    }
    assert(collectedBatch->count<=FACTOR_BATCH_COUNT && "Overfull factor batch");
    // TODO - what if both are done/empty at the same time? does that work correctly?
    // Factors full
    if (hitSummarizedBatch || collectedBatch->count>=FACTOR_BATCH_COUNT) {
        assert(collectedBatch->entries[0].prime != 0 && "Writing back garbage");
        // output filled batch
        ocrEventSatisfy(args->outputPromise, collectedBatchGuid);
        // Use current input batch, but new output factors batch
        ocrGuid_t depGuids[] = { inputBatchGuid, NULL_GUID, collectedBatch->requestNextBatch };
        assert(depc == sizeof(depGuids)/sizeof(*depGuids));
        // Update current input offset, and output "promise" guid
        args->batchOffset = batchOffset;
        args->outputPromise = collectedBatch->nextBatch;
        // recur, continuing work on current input batch (new factor batch)
        ocrGuid_t recursiveCallGuid;
        ocrEdtCreate(&recursiveCallGuid, args->recursiveTemplate, EDT_PARAM_DEF, paramv,
                EDT_PARAM_DEF, depGuids, EDT_PROP_NONE, NULL_GUID, NULL);
    }
    // Input primes batch exhausted
    else {
        assert(batchOffset==inputBatch->count && "Dropped input primes.");
        // Use current output factors batch, but new input batch
        ocrGuid_t depGuids[] = { inputBatch->nextBatch, collectedBatchGuid, NULL_GUID };
        assert(depc == sizeof(depGuids)/sizeof(*depGuids));
        // reset current offset
        args->batchOffset = 0;
        // recur, continuing to collect in current factor batch (new input batch)
        ocrGuid_t recursiveCallGuid;
        ocrEdtCreate(&recursiveCallGuid, args->recursiveTemplate, EDT_PARAM_DEF, paramv,
                EDT_PARAM_DEF, depGuids, EDT_PROP_NONE, NULL_GUID, NULL);
    }
    return NULL_GUID;
}

/* argument struct */
typedef struct {
    u32 summarizeFlag;
    uIntPrime N;
    uIntPrime base;
    uPrimeCount candidateCount;
    ocrGuid_t out;
    ocrGuid_t next;
    ocrGuid_t continueTemplateGuid;
} FilterParams;

bool scheduleNextFilter(FilterParams *args, ocrEdtDep_t cDep, ocrEdtDep_t fDep) {
    CandidateBatch *resultBatch = cDep.ptr;
    // Continue
    if (resultBatch->count > resultBatch->confirmedCount) {
        // Move to next batch of factors
        PrimeFactorBatch *factorBatch = fDep.ptr;
        // call next
        // request next (demand-driven)
        ocrEventSatisfy(factorBatch->requestNextBatch, NULL_GUID);
        // set up params
        ocrGuid_t depGuids[] = { cDep.guid, factorBatch->nextBatch };
        ocrGuid_t continueGuid;
        ocrEdtCreate(&continueGuid, args->continueTemplateGuid, EDT_PARAM_DEF, (u64*) args,
                EDT_PARAM_DEF, depGuids, EDT_PROP_NONE, NULL_GUID, NULL);
        return 1;
    }
    // Done
    else if (resultBatch->summarizeFlag == KEEP_NONE_FLAG) {
        ocrEdtDep_t batchDep;
        DBCREATE_BASIC(&batchDep.guid, (void**) &batchDep.ptr, sizeof(CandidateBatch));
        CandidateBatch *summarizedBatch = batchDep.ptr;
        *summarizedBatch = *resultBatch; // copy struct
        summarizedBatch->confirmedCount = 0; // summarized; count == confirmedCount implies full
        ocrEventSatisfy(args->out, batchDep.guid);
        ocrDbDestroy(cDep.guid); // release full batch's memory
        return 0;
    }
    else {
        if (resultBatch->summarizeFlag == KEEP_FACTOR_FLAG) {
            resultBatch->confirmedCount = 0; // summarized; count == confirmedCount implies full
        }
        ocrEventSatisfy(args->out, cDep.guid);
        return 0;
    }
}

ocrGuid_t continueFilterEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Read arguments
    FilterParams *args = (FilterParams*) paramv;
    CandidateBatch *candidateBatch = depv[0].ptr;
    PrimeFactorBatch *factorBatch = depv[1].ptr;
    // Filter first batch, using the entire range starting from `base'
    filterCandidates(candidateBatch, factorBatch);
    CHECK_CBATCH_COOKIE(candidateBatch);
    scheduleNextFilter(args, depv[0], depv[1]);
    return NULL_GUID;
}

ocrGuid_t startFilterEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Read arguments
    FilterParams *args = (FilterParams*) paramv;
    PrimeFactorBatch *factorBatch = depv[0].ptr;
    // Filter first batch, using the entire range starting from `base'
    ocrEdtDep_t result = filterCandidatesBase(args->base, factorBatch);
    CandidateBatch* batch = result.ptr;
    CHECK_CBATCH_COOKIE(batch);
    batch->nextBatch = args->next;
    batch->summarizeFlag = args->summarizeFlag;
    scheduleNextFilter(args, result, depv[0]);
    return NULL_GUID;
}

ocrGuid_t putNthPrimeEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    s64 N = paramv[0];
    ocrGuid_t nthPrimePromise = *(ocrGuid_t*)(paramv+1);
    CandidateBatch *batch = depv[0].ptr;
    PRINTF("Nth prime is from batch at %u\n", batch->baseValue);
    uIntPrime *nthPrime;
    ocrGuid_t nthPrimeGuid;
    DBCREATE_BASIC(&nthPrimeGuid, (void**) &nthPrime, sizeof(uIntPrime));
    *nthPrime = batch->entries[N-1]; // -1 for 0-based index
    ocrEventSatisfy(nthPrimePromise, nthPrimeGuid);
    return NULL_GUID;
}

ocrGuid_t getNthPrimeEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    assert(paramc == (2+QWORD_COUNT_OF(ocrGuid_t)) && depc == 1 && "Wrong # of args for getNthPrime");
    s64 N = paramv[0];
    ReducedResult *reducedPrimes = depv[0].ptr;
    N -= paramv[1];    // offset N by the seeded prime count
    assert(N < reducedPrimes->count && "Didn't find enough primes");
    N -= reducedPrimes->offset; // offset by all the summarized prime results
    for (u32 i=0; i<reducedPrimes->batchCount; i++) {
        BatchRef batch = reducedPrimes->batches[i];
        N -= batch.count;
        if (N <= 0) {
            N += batch.count;
            // schedule EDT to put nth prime with the found batch's guid
            ocrGuid_t putterTemplateGuid;
            ocrEdtTemplateCreate(&putterTemplateGuid, putNthPrimeEdt, paramc-1, 1);
            paramv[1] = N; // update N so I can reuse current paramv, minus first arg
            ocrGuid_t putterGuid;
            ocrEdtCreate(&putterGuid, putterTemplateGuid, EDT_PARAM_DEF, paramv+1,
                    EDT_PARAM_DEF, &batch.guid, EDT_PROP_NONE, NULL_GUID, NULL);
            return NULL_GUID;
        }
    }
    ERROR_DIE("Couldn't find Nth prime.");
}

ocrGuid_t combinePartialReductionsEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    ReducedResult *lhs = depv[0].ptr;
    ocrGuid_t lhsGuid = depv[0].guid;
    ReducedResult *rhs = depv[1].ptr;
    ocrGuid_t rhsGuid = depv[1].guid;
    return reduceBatches(lhs, lhsGuid, rhs, rhsGuid);
}

ocrGuid_t buildReductionTree(u32 depth, size_t count, ocrGuid_t *reductionPromises, ocrGuid_t cprTemplate) {
    assert(count > 0);
    if (count == 1) {
        return reductionPromises[0];
    }
    else if (count == 2) {
        ocrGuid_t edtGuid, outEvent = NULL_GUID;
        ocrEdtCreate(&edtGuid, cprTemplate, 0, NULL, 2, reductionPromises, EDT_PROP_NONE, NULL_GUID, &outEvent);
        assert(!IS_NULL_GUID(outEvent));
        return outEvent;
    }
    else {
        size_t newCount = (count + 1) / 2; // grouped by 2, rounded up
        for (u32 i=1, j=0; i<count; i+=2, j++) {
            assert(!IS_NULL_GUID(reductionPromises[i-1]) && !IS_NULL_GUID(reductionPromises[i]));
            ocrGuid_t out = buildReductionTree(depth+2000, 2, &reductionPromises[i-1], cprTemplate);
            DEBUG_ONLY(reductionPromises[i-1] = reductionPromises[i] = NULL_GUID);
            reductionPromises[j] = out;
        }
        if (count%2 == 1) {
            reductionPromises[newCount-1] = buildReductionTree(depth+1000, 1, &reductionPromises[count-1], cprTemplate);
        }
        DEBUG_ONLY(for (u32 i=newCount; i<count; i++) reductionPromises[i] = NULL_GUID);
        return buildReductionTree(depth+1, newCount, reductionPromises, cprTemplate);
    }
}

/**
 * Converts result of filters into a reduction node
 */
ocrGuid_t res2red(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t primesBatchGuid = depv[0].guid;
    CandidateBatch * primesBatch = depv[0].ptr;
    BatchRef batchRef = { primesBatch->count, primesBatchGuid };
    return reductionLeaf(primesBatch, &batchRef);
}

/* argument struct */
typedef struct {
    uPrimeCount N;
    ocrGuid_t nthPrimePromise;
} FindPrimesParams;

ocrGuid_t findPrimesEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    FindPrimesParams *args = (FindPrimesParams*) paramv;
    uPrimeCount N = args->N;
    PRINTF("Calculating the Nth prime for N=%lu...\n", (u64)N);

    // Storage for Nth prime result
    ocrGuid_t nthPrimeGuid;
    uIntPrime *nthPrime;
    DBCREATE_BASIC(&nthPrimeGuid, (void**) &nthPrime, sizeof(uIntPrime));

    // Trivial case: 1st or 2nd prime
    if (N < 3) { 
        *nthPrime = (N == 1) ? 2 : 3;
        ocrEventSatisfy(args->nthPrimePromise, nthPrimeGuid);
    }
    else {
        // Calculate seeds
        ocrEdtDep_t factorDep = primeSeeds(SEED_PRIMES_COUNT);
        PrimeFactorBatch *factorBatch = factorDep.ptr;
        assert(factorBatch->entries[127].prime == 733 && "130th prime = 733");

        // Trivial case: prime was seeded
        if (N-2 <= SEED_PRIMES_COUNT) {
            *nthPrime = factorBatch->entries[N-3].prime;
            ocrEventSatisfy(args->nthPrimePromise, nthPrimeGuid);
        }
        // Non-trivial case: need to keep computing primes
        else {
            // Calculate bounds
            uIntPrime pnUpperBound = primeUpperBound(N)+1; // +1 to make it exclusive
            uIntPrime pnLowerBound = primeLowerBound(N);
            uIntPrime factorBound = ZSQRT(pnUpperBound)+1;
            PRINTF("UB: %u LB: %u FB: %u\n", pnUpperBound, pnLowerBound, factorBound);
            //
            ocrGuid_t startTemplateGuid;
            u32 paramCount = QWORD_COUNT_OF(FilterParams);
            assert(paramCount * sizeof(u64) == sizeof(FilterParams));
            ocrEdtTemplateCreate(&startTemplateGuid, startFilterEdt, paramCount, 1);
            // Create batches
            uIntPrime nextBase = LAST_FACTOR(factorBatch).prime+2;
            // estimate how many segments are needed
            u32 primeBatchCount = (pnUpperBound - nextBase + CANDIDATE_BATCH_COUNT - 1) / CANDIDATE_BATCH_COUNT;
            PRINTF("Using %u batches\n", primeBatchCount);
            // This memory is only used within this EDT, so it's OK to malloc
            ocrGuid_t *batchGuids = MALLOC((primeBatchCount+1) * sizeof(ocrGuid_t));
            ocrGuid_t *filterEdts = MALLOC((primeBatchCount) * sizeof(ocrGuid_t));
            for (u32 i=0; i<primeBatchCount; i++) {
                ocrEventCreate(&batchGuids[i], OCR_EVENT_STICKY_T, true);
            }
            batchGuids[primeBatchCount] = NULL_GUID; // This makes the ".next" logic easier
            // Set up template
            ocrGuid_t continueTemplateGuid;
            ocrEdtTemplateCreate(&continueTemplateGuid, continueFilterEdt, QWORD_COUNT_OF(FilterParams), 2);
            // start filters
            for (u32 i=0; i<primeBatchCount; i++) {
                // Check if the resulting primes need to be kept around
                uIntPrime nextUBound = nextBase+CANDIDATE_BATCH_COUNT;
                bool inFactorRange = rangesOverlap(0, factorBound, nextBase, nextUBound);
                bool inTargetRange = rangesOverlap(nextBase, nextUBound, pnLowerBound, pnUpperBound*2);
                u32 factorFlag = inFactorRange ? KEEP_FACTOR_FLAG : 0;
                u32 targetFlag = inTargetRange ? KEEP_TARGET_FLAG : 0;
                // Set up parameters
                FilterParams params = {
                    .summarizeFlag = factorFlag | targetFlag,
                    .N = N,
                    .base = nextBase,
                    .candidateCount = MIN(CANDIDATE_BATCH_COUNT, pnUpperBound-nextBase),
                    .out = batchGuids[i],
                    .next = batchGuids[i+1],
                    .continueTemplateGuid = continueTemplateGuid
                };
                // Start filter task
                ocrEdtCreate(&filterEdts[i], startTemplateGuid, EDT_PARAM_DEF, (u64*) &params,
                        EDT_PARAM_DEF, NULL, EDT_PROP_NONE, NULL_GUID, NULL);
                // next
                nextBase += CANDIDATE_BATCH_COUNT;
            }
            // Collect factors (on-demand)
            {
                u32 paramCount = QWORD_COUNT_OF(FactorCollectorParams);
                ocrGuid_t collectorTemplateGuid;
                ocrEdtTemplateCreate(&collectorTemplateGuid, collectFactorsEdt, paramCount, 3);
                FactorCollectorParams collectorParams = {
                    .batchOffset = 0,
                    .outputPromise = factorBatch->nextBatch,
                    .recursiveTemplate = collectorTemplateGuid
                };
                ocrGuid_t deps[] = { batchGuids[0], NULL_GUID, factorBatch->requestNextBatch };
                ocrGuid_t collectorGuid;
                ocrEdtCreate(&collectorGuid, collectorTemplateGuid, EDT_PARAM_DEF, (u64*)&collectorParams,
                        EDT_PARAM_DEF, deps, EDT_PROP_NONE, NULL_GUID, NULL);
            }
            // Reduce for nth prime
            {
                // map res2red across outputs to get right type for reducer
                ocrGuid_t res2redTemplateGuid;
                ocrEdtTemplateCreate(&res2redTemplateGuid, res2red, 0, 1);
                for (u32 i=0; i<primeBatchCount; i++) {
                    ocrGuid_t res2redGuid, out;
                    ocrEdtCreate(&res2redGuid, res2redTemplateGuid, EDT_PARAM_DEF, NULL,
                            EDT_PARAM_DEF, &batchGuids[i], EDT_PROP_NONE, NULL_GUID, &out);
                    batchGuids[i] = out;
                }
                // Build reduction tree
                ocrGuid_t cprTemplateGuid;
                ocrEdtTemplateCreate(&cprTemplateGuid, combinePartialReductionsEdt, 0, 2);
                ocrGuid_t treeRoot = buildReductionTree(0, primeBatchCount, batchGuids, cprTemplateGuid);
                // Set up nthPrimeEdt
                u32 paramCount = 2+QWORD_COUNT_OF(ocrGuid_t);
                u64 nthPrimeParams[paramCount];
                nthPrimeParams[0] = N;
                nthPrimeParams[1] = SEED_PRIMES_COUNT + 2; // +2 is for 2 and 3
                *(ocrGuid_t*)(nthPrimeParams+2) = args->nthPrimePromise;
                ocrGuid_t nthPrimeTemplateGuid;
                ocrEdtTemplateCreate(&nthPrimeTemplateGuid, getNthPrimeEdt, paramCount, 1);
                // start it waiting on the reducer tree root result
                ocrGuid_t nthPrimeGuid;
                ocrEdtCreate(&nthPrimeGuid, nthPrimeTemplateGuid, EDT_PARAM_DEF, nthPrimeParams,
                        EDT_PARAM_DEF, &treeRoot, EDT_PROP_NONE, NULL_GUID, NULL);
            }
            // It's safe to start the computation now that all the events/deps are set up
            for (u32 i=0; i<primeBatchCount; i++) {
                ocrAddDependence(factorDep.guid, filterEdts[i], 0, DB_MODE_RO);
            }
            // free EDT-local memory
            FREE(filterEdts);
            FREE(batchGuids);
        }
        assert(factorBatch->entries[127].prime == 733 && "130th prime = 733");
    }
    return NULL_GUID;
}

ocrGuid_t exitEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 N = paramv[0];
    u64 nthPrime = *(uIntPrime*)depv[0].ptr;
    PRINTF("The Nth prime where N=%lu is %lu.\n", (u64)N, (u64)nthPrime);
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {

    u64 N = parseNArg(depv[0].ptr);

    // Event (promise/box) for final Nth prime result
    ocrGuid_t nthPrimePromiseGuid;
    ocrEventCreate(&nthPrimePromiseGuid, OCR_EVENT_ONCE_T, true);

    // Find the primes
    FindPrimesParams params = { .N = N, .nthPrimePromise = nthPrimePromiseGuid };
    ocrGuid_t findPrimesTemplateGuid;
    ocrEdtTemplateCreate(&findPrimesTemplateGuid, findPrimesEdt, QWORD_COUNT_OF(params), 0);
    ocrGuid_t findPrimesGuid;
    ocrEdtCreate(&findPrimesGuid, findPrimesTemplateGuid, EDT_PARAM_DEF, (u64*) &params,
            EDT_PARAM_DEF, NULL, EDT_PROP_NONE, NULL_GUID, NULL);

    // EDT wraps up after all primes computation is finded (depends on the nthPrimePromise)
    ocrGuid_t exitTemplateGuid;
    ocrEdtTemplateCreate(&exitTemplateGuid, exitEdt, 1, 1);
    ocrGuid_t exitGuid;
    ocrEdtCreate(&exitGuid, exitTemplateGuid, EDT_PARAM_DEF, &N,
            EDT_PARAM_DEF, &nthPrimePromiseGuid, EDT_PROP_NONE, NULL_GUID, NULL);

    return NULL_GUID;
}

