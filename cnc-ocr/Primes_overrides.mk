# Use C99 extensions
CC_OPTS+=-std=gnu99

# add custom sources
SRCS+=primes_common.c

# update CFLAGS based on some make flags
include ../common/Makefile.common
