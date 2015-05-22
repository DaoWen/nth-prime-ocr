#include "Primes.h"

extern void nextGridFor(u32 totalWidth, u32 span, u32 row, u32 col, cncTag_t *rowOut, cncTag_t *colOut);

/**
 * Converts result of filters into a reduction node
 */
ReducedResult *reductionLeaf(CandidatesInfo *batch, BatchRef *batchRef) {
    u32 batchLimit = IS_SUMMARY_BATCH(batch) ? 0 : 16;
    // TODO - technically we could have empty batches if we encounter huge gaps
    ASSERT(batch->count > 0 && "Can't have empty batches!");
    ReducedResult *red = cncItemCreateSized_reduced(sizeof(ReducedResult) + batchLimit*sizeof(BatchRef));
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
    return red;
}

void Primes_makeReducerLeafStep(cncTag_t width, cncTag_t row, cncTag_t col, CandidatesInfo *primesInfo, PrimesCtx *ctx) {
    //DEBUG_LOG("Making leaf at %u %u (w=%u)\n", row, col, width);
    BatchRef batchRef = { primesInfo->count, col };
    nextGridFor(width, 1, row, col, &row, &col);
    cncPut_reduced(reductionLeaf(primesInfo, &batchRef), row, col, ctx);
    if (col % 2 == 0) {
        cncPrescribe_reducerStep(width, 2, row-1, col/2, ctx);
    }
}
