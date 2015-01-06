# Makefile for MinMon
#----------------------------------------------------------------------------
# 22-Dec-2014 ShaneG
#
# Makefile to build MinMon. At this stage this is tied to the CC65 compiler
# (see http://cc65.github.io/cc65/ for more information).
#----------------------------------------------------------------------------

# Base file name for output
FILENAME = baselib

# Define tools
CC = cl65
AS = ca65
AR = ar65

# Flags for tools
CFLAGS = -t none

# Build the list of object files needed
OBJECTS = $(patsubst %.s,%.o,$(wildcard *.s))
OBJECTS += $(patsubst %.c,%.o,$(wildcard *.c))

all: $(FILENAME).lib

clean:
	rm -f $(OBJECTS) $(FILENAME).lib

# How to build the binary
$(FILENAME).lib: $(OBJECTS)
	$(AR) a $@ $(OBJECTS)

# How to build object files from C source
%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

# How to build object files from assembly
%.o:%.s
	$(AS) $(AFLAGS) -o $@ $^

