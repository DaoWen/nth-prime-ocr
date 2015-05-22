#!/bin/bash

TMPFILE=.tmp.out

# Guess CNCOCR_ROOT if it's not already set
export CNCOCR_ROOT="${CNCOCR_ROOT-${XSTACK_ROOT}/hll/cnc}"
CNCOCR_BIN="${CNCOCR_ROOT}/bin"

# Sanity check on the above paths
if ! [ -f "${CNCOCR_ROOT}/bin/cncocr_t" ]; then
    echo "ERROR: Failed to find CNCOCR_ROOT (looking in '${CNCOCR_ROOT}')"
    echo "       You need to set XSTACK_ROOT to the correct path"
    exit 1
fi

# Run the translator if needed
[ -d "cncocr_support" ] || "${CNCOCR_BIN}/cncocr_t"

# Build optimized
[ ! -e Primes ] && make OPT=2

echo "Testing N=300"
make run 300 2>&1 | tee $TMPFILE
fgrep -q 'The Nth prime where N=300 is 1987.' < $TMPFILE  && echo OK || echo FAIL

echo "Testing N=1,000,000"
make run 1000000 2>&1 | tee $TMPFILE
fgrep -q 'The Nth prime where N=1000000 is 15485863.' < $TMPFILE  && echo OK || echo FAIL

rm $TMPFILE
