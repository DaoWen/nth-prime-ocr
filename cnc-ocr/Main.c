#include "Primes.h"

ocrGuid_t mainEdt(u32 paramc, u64 paramv[], u32 depc, ocrEdtDep_t depv[]) {

    // Read argument N for Nth prime
    CNC_REQUIRE(OCR_MAIN_ARGC==2, "Expected argument N to calculate Nth prime.\n");
    uPrimeCount N = atoi(OCR_MAIN_ARGV(1));
    CNC_REQUIRE(N>0, "Value for N is not in range: %s\n", OCR_MAIN_ARGV(1));

    // Create a new graph context
    PrimesCtx *context = Primes_create();
    context->n = N;

    // Launch the graph for execution
    Primes_launch(NULL, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINALIZE(context);

    return NULL_GUID;
}
