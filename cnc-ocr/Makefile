TARGET=Primes.exec
CFLAGS=-std=gnu99 -g -I$(CNCOCR_INSTALL)/include -I$(OCR_INSTALL)/include -D__OCR__
WARNFLAGS=-Werror -Wall -Wextra -Wno-unused-parameter
CFLAGS+=$(WARNFLAGS) -DPRIMES_APP_CNC_MODE

# Compile all generated step codes
include steplist.mk
SRCS=Main.c Common.c Context.c Dispatch.c $(STEP_SRCS) primes_common.c
OBJS=$(patsubst %.c,%.o,$(SRCS))

# update CFLAGS based on some make flags
include ../common/Makefile.common

# shut down CnC runtime even if some steps never finish
CFLAGS+=-DCNC_UNCLEAN_FINISH

# include header globally for user-defined types
CFLAGS+=-include primes_common.h

compile: $(TARGET)

# building source files
%.o: %.c
	gcc $(CFLAGS) -c $<

# linking - creating the executable
$(TARGET): $(OBJS)
	gcc $(CFLAGS) \
	       -L"$(OCR_INSTALL)/lib" \
	       -L"$(CNCOCR_INSTALL)/lib" \
	       $(OBJS) \
	       -o $(TARGET) -locr -lcncocr -lm

# delete binaries
clean:
	rm -f $(OBJS) $(TARGET)

# delete binaries, scaffolding and output files
squeaky: clean
	rm -f {Context,Dispatch,Common}.[ch] steplist.mk

