////////////////////////////////////////////////////////////////////////////////
// @author: Nick Vrvilo
// nick.vrvilo@rice.edu
////////////////////////////////////////////////////////////////////////////////

$context {
    uPrimeCount n;
};

////////////////////////////////////////////////////////////////////////////////
// item collection declarations

[ uIntPrime        *candidates:     i, j ]; // partially-filtered prime candidates
[ CandidatesInfo   *candidatesInfo: i, j ]; // meta-data on prime candidate batches
[ uIntPrime        *primes:         i    ]; // confirmed prime numbers
[ CandidatesInfo   *primesInfo:     i    ]; // meta-data on batches of confirmed primes
[ void             *factorsReq:     i    ]; // request the next group of factors
[ PrimeFactor      *factors:        i    ]; // fully-grouped prime factors
[ PrimeFactor      *collected:      i, j ]; // partially-grouped prime factors
[ ReducedResult    *reduced:        r, c ]; // counted batches of primes in reduction tree
[ uIntPrime         nthPrime:       ()   ]; // final result is the nth prime number

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Input output relationships

// filter start
( filterStartStep: i, keep, base, count )
 <- [ factors: 0 ]
 -> [ candidates: i, 1 ],         // if continuing
    [ candidatesInfo: i, 1 ],     // if continuing
    ( filterContinueStep: i, 1 ), // if continuing
    [ primes: i ],                // if ending
    [ primesInfo: i ];            // if ending

// filter continue
( filterContinueStep: i, j )
 <- [ factors: j ], [ candidatesInfo: i, j ], [ candidates: i, j ]
 -> [ candidates: i, j+1 ],        // if continuing
    [ candidatesInfo: i, j+1 ],    // if continuing
    ( filterContinueStep: i, j+1 ), // if continuing
    [ primes: i ],                 // if ending
    [ primesInfo: i ];             // if ending

// collect factors into nice-sized batches
( collectFactorsStep: i, j, offset, count )
 <- [ collected: i, j ], [ primesInfo: j ], [ primes: j ], [ factorsReq: i ]
 -> [ collected: i+1, j ], ( collectFactorsStep: i+1, j, offset, 0 ), [ factors: i ],
    [ collected: i, j+1 ], ( collectFactorsStep: i, j+1, 0, count );

// reducer setup
( makeReducerLeafStep: width, row, col )
 <- [ primesInfo: col ]
 -> [ reduced: row, col ];

// reducer
( reducerStep: width, span, row, col )
 <- [ lhs @ reduced: row+1, col*2 ], [ rhs @ reduced: row+1, col*2+1 ]
 -> [ reduced: row, col ],
    ( reducerStep: width, span*2, row/2, col/2 );

( findTargetBatchStep: n )
 <- [ reduced: 0, 0 ]
 -> ( findNthPrimeStep: n, i );

( findNthPrimeStep: n, i )
 <- [ primes: i ]
 -> [ nthPrime: () ];

// Write graph inputs and start steps
( $init : () )
 -> [ factors : 0 ], ( filterStartStep : 0, keep, base, count ),
    ( reducerStep : treeHeight-1, $range(treeWidth/2) ), ( findTargetBatchStep : n );

// Return outputs to the caller
// (doesn't really work like that with OCR)
( $finalize: () ) <- [ nthPrime: () ];

