////////////////////////////////////////////////////////////////////////////////
// @author: Nick Vrvilo
// nick.vrvilo@rice.edu
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// item collection declarations

[ uIntPrime        * candidates     ]; // partially-filtered prime candidates
[ CandidatesInfo   * candidatesInfo ]; // meta-data on prime candidate batches
[ uIntPrime        * primes         ]; // confirmed prime numbers
[ CandidatesInfo   * primesInfo     ]; // meta-data on batches of confirmed primes
[ void             * factorsReq     ]; // request the next group of factors
[ PrimeFactor      * factors        ]; // fully-grouped prime factors
[ PrimeFactor      * collected      ]; // partially-grouped prime factors
[ ReducedResult    * reduced        ]; // counted batches of primes in reduction tree
[ uIntPrime          nthPrime       ]; // final result is the nth prime number

////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// tags declarations 

< u32 [4] filterStartTag >;
< u32 [2] filterContinueTag >;

< u32 [4] collectFactorsTag >;

< u32 [3] makeReducerLeafTag >;
< u32 [4] reducerTag >;

< u32 [1] findTargetBatchTag >;
< u32 [2] findNthPrimeTag >;


////////////////////////////////////////////////////////////////////////////////
// Step Prescriptions

< filterStartTag > :: ( filterStartStep );
< filterContinueTag > :: ( filterContinueStep );

< collectFactorsTag > :: ( collectFactorsStep );

< makeReducerLeafTag > :: ( makeReducerLeafStep );
< reducerTag > :: ( reducerStep );

< findTargetBatchTag > :: ( findTargetBatchStep );
< findNthPrimeTag > :: ( findNthPrimeStep );


////////////////////////////////////////////////////////////////////////////////
// Input output relationships

// filter start
[ factors: 0 ] -> ( filterStartStep: i, keep, base, count );
( filterStartStep: i, keep, base, count ) -> [ candidates: i, 1 ];        // if continuing
( filterStartStep: i, keep, base, count ) -> [ candidatesInfo: i, 1 ];    // if continuing
( filterStartStep: i, keep, base, count ) -> < filterContinueTag: i, 1 >; // if continuing
( filterStartStep: i, keep, base, count ) -> [ primes: i ];               // if ending
( filterStartStep: i, keep, base, count ) -> [ primesInfo: i ];           // if ending

// filter continue
[ factors: j ], [ candidatesInfo: i, j ], [ candidates: i, j ] -> ( filterContinueStep: i, j );
( filterContinueStep: i, j ) -> [ candidates: i, j+1 ];        // if continuing
( filterContinueStep: i, j ) -> [ candidatesInfo: i, j+1 ];    // if continuing
( filterContinueStep: i, j ) -> < filterContinueTag: i, j+1 >; // if continuing
( filterContinueStep: i, j ) -> [ primes: i ];                 // if ending
( filterContinueStep: i, j ) -> [ primesInfo: i ];             // if ending

// collect factors into nice-sized batches
[ collected: i, j ], [ primesInfo: j ], [ primes: j ], [ factorsReq: i ] -> ( collectFactorsStep: i, j, offset, count );
( collectFactorsStep: i, j, offset, count ) -> [ collected: i+1, j ], < collectFactorsStep: i+1, j, offset, 0 >, [ factors: i ];
( collectFactorsStep: i, j, offset, count ) -> [ collected: i, j+1 ], < collectFactorsStep: i, j+1, 0, count >;

// reducer setup
[ primesInfo: col ] -> ( makeReducerLeafStep: width, row, col );
( makeReducerLeafStep: width, row, col ) -> [ reduced: row, col ];

// reducer
[ reduced: row+1, col*2 ], [ reduced: row+1, col*2+1 ] -> ( reducerStep: width, span, row, col );
( reducerStep: width, span, row, col ) -> [ reduced: row, col ];
( reducerStep: width, span, row, col ) -> < reducerTag: width, span*2, row/2, col/2 >;

[ reduced: 0, 0 ] -> ( findTargetBatchStep: n );
( findTargetBatchStep: n ) -> < findNthPrimeTag: n, i >;

[ primes: i ] -> ( findNthPrimeStep: n, i );
( findNthPrimeStep: n, i ) -> [ nthPrime: 0 ];

// Write graph inputs and start steps
env -> [ factors : 0 ], < filterStartTag : 0, keep, base, count >;
env -> < reducerTag : treeHeight-1, {0 .. treeWidth/2} >, < findTargetBatchTag : n >;

// Return outputs to the caller
// (doesn't really work like that with OCR)
[ nthPrime: 0 ] -> ( env: n );
