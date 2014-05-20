
#include "Common.h"

void nextGridFor(u32 totalWidth, u32 span, u32 row, u32 col, u32 *rowOut, u32 *colOut);

/**
 * Converts result of filters into a reduction node
 */
cncHandle_t reductionLeaf(CandidatesInfo *batch, BatchRef *batchRef) {
    ReducedResult *red;
    u32 batchLimit = IS_SUMMARY_BATCH(batch) ? 0 : 16;
    // TODO - technically we could have empty batches if we encounter huge gaps
    ASSERT(batch->count > 0 && "Can't have empty batches!");
    cncHandle_t redHandle = cncCreateItemSized_reduced(&red, sizeof(ReducedResult) + batchLimit*sizeof(BatchRef));
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
    return redHandle;
}

void makeReducerLeafStep( u32 width, u32 row, u32 col, primesInfoItem primesInfo, Context *context){
    //DEBUG_LOG("Making leaf at %u %u (w=%u)\n", row, col, width);
    BatchRef batchRef = { primesInfo.item->count, col };
    nextGridFor(width, 1, row, col, &row, &col);
    cncPut_reduced(reductionLeaf(primesInfo.item, &batchRef), row, col, context);
    if (col % 2 == 0) {
        cncPrescribe_reducerStep(width, 2, row-1, col/2, context);
    }
}

