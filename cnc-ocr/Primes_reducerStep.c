#include "Primes.h"

void nextGridFor(u32 totalWidth, u32 span, u32 row, u32 col, cncTag_t *rowOut, cncTag_t *colOut) {
    if ((col+1)*span >= totalWidth) {
        while (col > 0 && col%2 == 0) {
            col /= 2;
            row--;
        }
    }
    *rowOut = row;
    *colOut = col;
}

/**
 * Step function defintion for "reducerStep"
 */
void reducerStep(cncTag_t width, cncTag_t span, cncTag_t row, cncTag_t col, ReducedResult *lhs, ReducedResult *rhs, PrimesCtx *ctx) {
    // Combine lhs and rhs
    ReducedResult *target;
    assert(lhs != rhs && "Aliased inputs");
    assert(lhs != NULL && rhs != NULL && "Shouldn't have null inputs to combine");
    assert((lhs->batchLimit == 0 || rhs->batchLimit > 0) && "Can't have full lhs and summarized rhs");
    uPrimeCount newBatchCount = lhs->batchCount + rhs->batchCount;
    // Case: both fit in rhs's allocated space
    if (rhs->batchLimit >= newBatchCount) {
        target = rhs;
    }
    // Case: need to allocate a new block
    else {
        assert(rhs->batchLimit > 0);
        size_t newBatchLimit = rhs->batchLimit;
        while (newBatchLimit < newBatchCount) newBatchLimit *= 2;
        size_t newSize = sizeof(ReducedResult)+newBatchLimit*sizeof(BatchRef);
        target = cncCreateItemSized_reduced(newSize);
        target->batchLimit = newBatchLimit;
    }
    // copy rhs's entries (careful! target and rhs may be aliased!)
    memmove(target->batches+lhs->batchCount, rhs->batches, sizeof(BatchRef)*rhs->batchCount);
    // copy lhs's entries
    memcpy(target->batches, lhs->batches, sizeof(BatchRef)*lhs->batchCount);
    // update counts
    target->count = rhs->count + lhs->count;
    target->offset = rhs->offset + lhs->offset;
    target->batchCount = newBatchCount;
    // clean up
    assert(lhs != target);
    cncFree(lhs); // free lhs memory
    if (rhs != target) cncFree(rhs); // free rhs memory if not used
    // Output
    nextGridFor(width, span, row, col, &row, &col);
    if (col%2 == 0 && row > 0) {
        // Combine this instance with the right neighbor (i.e. col+1)
        cncPrescribe_reducerStep(width, span*2, row-1, col/2, ctx);
    }
    cncPut_reduced(target, row, col, ctx);
    //DEBUG_LOG("Put to %u %u (s=%u)\n", row, col, span);
}
