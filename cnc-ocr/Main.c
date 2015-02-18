#include "Primes.h"

int cncMain(int argc, char *argv[]) {

    // Read argument N for Nth prime
    CNC_REQUIRE(argc==2, "Expected argument N to calculate Nth prime.\n");
    uPrimeCount N = atoi(argv[1]);
    CNC_REQUIRE(N>0, "Value for N is not in range: %s\n", argv[1]);

    // Create a new graph context
    PrimesCtx *context = Primes_create();
    context->n = N;

    // Launch the graph for execution
    Primes_launch(NULL, context);

    // Exit when the graph execution completes
    CNC_SHUTDOWN_ON_FINALIZE(context);

    return 0;
}
