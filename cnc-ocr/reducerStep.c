
#include "Common.h"

void nextGridFor(u32 totalWidth, u32 span, u32 row, u32 col, u32 *rowOut, u32 *colOut) {
    if ((col+1)*span >= totalWidth) {
        while (col > 0 && col%2 == 0) {
            col /= 2;
            row--;
        }
    }
    *rowOut = row;
    *colOut = col;
}

void reducerStep(u32 width, u32 span, u32 row, u32 col, reducedItem iLhs, reducedItem iRhs, Context *context) {
    // Combine lhs and rhs
    cncHandle_t targetHandle;
    ReducedResult *target, *lhs = iLhs.item, *rhs = iRhs.item;
    assert(lhs != rhs && "Aliased inputs");
    assert(lhs != NULL && rhs != NULL && "Shouldn't have null inputs to combine");
    assert((lhs->batchLimit == 0 || rhs->batchLimit > 0) && "Can't have full lhs and summarized rhs");
    uPrimeCount newBatchCount = lhs->batchCount + rhs->batchCount;
    // Case: both fit in rhs's allocated space
    if (rhs->batchLimit >= newBatchCount) {
        target = rhs;
        targetHandle = iRhs.handle;
    }
    // Case: need to allocate a new block
    else {
        assert(rhs->batchLimit > 0);
        size_t newBatchLimit = rhs->batchLimit;
        while (newBatchLimit < newBatchCount) newBatchLimit *= 2;
        size_t newSize = sizeof(ReducedResult)+newBatchLimit*sizeof(BatchRef);
        targetHandle = cncCreateItemSized_reduced(&target, newSize);
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
    assert(iLhs.handle != targetHandle);
    CNC_DESTROY_ITEM(iLhs.handle); // free lhs memory
    if (iRhs.handle != targetHandle) CNC_DESTROY_ITEM(iRhs.handle); // free rhs memory if not used
    // Output
    nextGridFor(width, span, row, col, &row, &col);
    if (col%2 == 0 && row > 0) {
        // Combine this instance with the right neighbor (i.e. col+1)
        cncPrescribe_reducerStep(width, span*2, row-1, col/2, context);
    }
    cncPut_reduced(targetHandle, row, col, context);
    //DEBUG_LOG("Put to %u %u (s=%u)\n", row, col, span);
}

