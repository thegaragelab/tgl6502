# Makefile for MinMon
#----------------------------------------------------------------------------
# 22-Dec-2014 ShaneG
#
# Makefile to build MinMon. At this stage this is tied to the CC65 compiler
# (see http://cc65.github.io/cc65/ for more information).
#----------------------------------------------------------------------------

# Base file name for output
FILENAME = minmon

# Define tools
CC = cl65
LD = ld65
AS = ca65

# Define options
CFLAGS  = -t none
LDFLAGS = -C ../../mos6502/rom16.cfg -m $(FILENAME).map

# Build the list of object files based on the selections
OBJECTS = ../../mos6502/romcrt0.o
OBJECTS += $(patsubst %.c,%.o,$(wildcard *.c))

all: $(FILENAME).rom

clean:
	rm -f $(OBJECTS) $(FILENAME).bin

# How to build the binary
$(FILENAME).rom: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) ../../mos6502/runtime.lib

# How to build object files from C source
%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $^

# How to build object files from assembly
%.o:%.s
	$(AS) $(ASFLAGS) -o $@ $^
