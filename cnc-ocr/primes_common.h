#ifndef NV_PRIMES_COMMON_H
#define NV_PRIMES_COMMON_H


/*******************************/
/* Shared includes and defines */
/*******************************/

#ifndef __OCR__
#define __OCR__
#endif

#include "ocr.h"
#include "DataDriven.h"
#include <string.h>


/****************************/
/* USER CONFIGURED SETTINGS */
/****************************/

typedef u32 uIntPrime;         // Must fit a prime candidate
typedef u64 uIntPrimeScaled;   // Must fit a scaled reciprocal of a prime candidate
typedef uIntPrime uPrimeCount; // Should be the same size as uIntPrime
#define CANDIDATE_BATCH_COUNT  (1024U*8)
#define FACTOR_BATCH_COUNT     128


/**************************/
/* General utility macros */
/**************************/

#define COMP_EXP(a, b, op) ({ typeof(a) x = a; typeof(b) y = b; x op y ? x : y; })
#define MIN(a, b) COMP_EXP(a, b, <)
#define MAX(a, b) COMP_EXP(a, b, >)

#define ZSQRT(x) ((uIntPrime)sqrt(x) + 1)

#ifdef NDEBUG // Assertions off
#define DEBUG 0
#else
#define DEBUG 1
#endif // NDEBUG
#define DEBUG_ONLY(x) do { if (DEBUG) { x; } } while (0)

#ifdef DEBUG_LOGGING
#define DEBUG_LOG(...) PRINTF(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif


/**********************/
/* OCR utility macros */
/**********************/

#define ERROR_DIE(...) do { PRINTF(__VA_ARGS__); ocrShutdown(); exit(-1); } while (0)


/**********/
/* Primes */
/**********/

#define UINTP_MAX           (~(uIntPrime)0)
#define UINTP_MAX_ROOT      ZSQRT(UINTP_MAX)


/*****************/
/* Prime factors */
/*****************/

#ifndef USE_PRIME_RECIPS
#define USE_PRIME_RECIPS 0
#endif

#if USE_PRIME_RECIPS // Using reciprocals of factors
#define PRIME_SCALE_FACTOR (8*sizeof(uIntPrime))
#define PRIME_RECIP_ONLY(line) line
#define DIVIDES(factor, candidate) ( (((candidate)*(factor).recip)>>PRIME_SCALE_FACTOR)*(factor).prime == (candidate) )
#define PRIME_RECIP_OF(x) ((((uIntPrimeScaled)1)<<PRIME_SCALE_FACTOR)/(x)+1)
#define PRIME_FACTOR_OF(x) (PrimeFactor){.prime=(x), .recip=PRIME_RECIP_OF(x)}
#else // Using mod operation
#define PRIME_RECIP_ONLY(line)
#define DIVIDES(factor, candidate) ( (candidate)%(factor).prime == 0 )
#define PRIME_FACTOR_OF(x) (PrimeFactor){.prime = (x)}
#endif // USE_PRIME_RECIPS

#define LAST_FACTOR(fs) ((fs)[FACTOR_BATCH_COUNT-1])    

typedef struct {
    uIntPrime prime;
    PRIME_RECIP_ONLY(uIntPrimeScaled recip);
} PrimeFactor;


/***************************/
/* Prime candidate batches */
/***************************/

#define CANDIDATE_SLOTS         (CANDIDATE_BATCH_COUNT/3 + 4) // Down to 1/3 size after trying just 2 and 3

// Bit flags for whether to keep full results
#define KEEP_NONE_FLAG   0 // Can throw out everything
#define KEEP_FACTOR_FLAG 1 // Keep for factors batching
#define KEEP_TARGET_FLAG 2 // Within the nth prime range

#define IS_SUMMARY_BATCH(b) (((b)->summarizeFlag & KEEP_TARGET_FLAG) == 0)
#define IS_FACTOR_BATCH(b)  (((b)->summarizeFlag & KEEP_FACTOR_FLAG) != 0)

typedef struct {
    u32 summarizeFlag;
    u32 count;
    u32 confirmedCount;
    uIntPrime baseValue;
} CandidatesInfo;


/**************************/
/* Reduction tree results */
/**************************/

typedef struct {
    u32 count;
    u32 index;
} BatchRef;

typedef struct {
    uPrimeCount count;
    uIntPrime offset;
    uPrimeCount batchCount;
    uPrimeCount batchLimit;
    BatchRef batches[];
} ReducedResult;


/*********************/
/* Helper prototypes */
/*********************/

uIntPrime primeUpperBound(uPrimeCount x);
uIntPrime primeLowerBound(uPrimeCount x);
uPrimeCount piUpperBound(uIntPrime x);

void putNthPrime(uPrimeCount n, uIntPrime nthPrime, struct Context *context);
cncHandle_t primeSeeds(PrimeFactor **factorsDest);

#endif /* NV_PRIMES_COMMON_H */
