# Make sure we can find OCR
ifndef OCR_INSTALL
ifndef XSTACK_SRC
$(error Please define XSTACK_SRC or OCR_INSTALL)
endif
OCR_TYPE ?= x86-pthread-x86
OCR_INSTALL=$(XSTACK_SRC)/ocr/install/$(OCR_TYPE)
endif
OCR_HOME=$(shell dirname `dirname $(OCR_INSTALL)`)
OCR_AR=$(OCR_INSTALL)/lib/libocr.a

# Auto-parse arguments for "make run"
WORKLOAD_INPUT_FILE_IDX := 0
-include $(XSTACK_SRC)/apps/makefiles/make-pre.inc

WARNFLAGS=-Wall -Wextra -Wno-unused-parameter
# Edit ~/xstack/ocr/build/common.mk to change the debug level for the OCR build
CFLAGS=$(WARNFLAGS) -g -DOCR_DEBUG -DOCR_DEBUG_LVL=DEBUG_LVL_INFO -I$(OCR_INSTALL)/include -D__OCR__
LDFLAGS=-L$(OCR_INSTALL)/lib $(OCR_AR) -lm

# update CFLAGS based on some make flags
include ../common/Makefile.common

# Edit ~/xstack/ocr/build/common.mk to change the deque size
# Add this line: CFLAGS += -DINIT_DEQUE_CAPACITY='(1024*8)'
# That 8k deque size works for N=2,000,000 if candidate batch count is 8k

compile: primes

# This is including the header file in the gcc thing
primes: primes.c primes_common.h
	gcc -pthread -std=gnu99 $(CFLAGS) $< -o $@ $(LDFLAGS)

rebuild: clean primes

clean:
	rm -f primes

CFG_NAME ?= mach-hc-8w-lockableDB.cfg
OCR_CONFIG ?= $(OCR_INSTALL)/config/$(CFG_NAME)

run: primes
	OCR_CONFIG=$(OCR_CONFIG) ./primes $(WORKLOAD_ARGS)

dbg: primes
	OCR_CONFIG=$(OCR_CONFIG) gdb -ex r --args ./primes $(WORKLOAD_ARGS)
