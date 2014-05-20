WARNFLAGS=-Wall -Wextra -Wno-unused-parameter
# Edit ~/xstack/ocr/build/common.mk to change the debug level for the OCR build
CFLAGS=$(WARNFLAGS) -g -DOCR_DEBUG -DOCR_DEBUG_LVL=DEBUG_LVL_INFO -I$(OCR_INSTALL)/include -D__OCR__
LDFLAGS=-L$(OCR_INSTALL)/lib -locr -lm

# update CFLAGS based on some make flags
include ../common/Makefile.common

# Edit ~/xstack/ocr/build/common.mk to change the deque size
# Add this line: CFLAGS += -DINIT_DEQUE_CAPACITY='(1024*8)'
# That 8k deque size works for N=2,000,000 if candidate batch count is 8k

compile: primes

# This is including the header file in the gcc thing
primes: primes.c primes_common.h
	gcc --std=gnu99 $(CFLAGS) $(LDFLAGS) $< -o $@

rebuild: clean primes

clean:
	rm -f primes

run: primes
	./primes $(ARGS)

dbg: primes
	gdb ./primes

