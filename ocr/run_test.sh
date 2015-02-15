#!/bin/bash

TMPFILE=.tmp.out

# Call translator and make Primes binary
[ ! -e primes ] && make OPT=2

echo "Testing N=300"
make run 300 2>&1 | tee $TMPFILE
fgrep -q 'The Nth prime where N=300 is 1987.' < $TMPFILE  && echo OK || echo FAIL

echo "Testing N=1,000,000"
make run 1000000 2>&1 | tee $TMPFILE
fgrep -q 'The Nth prime where N=1000000 is 15485863.' < $TMPFILE  && echo OK || echo FAIL

rm $TMPFILE
