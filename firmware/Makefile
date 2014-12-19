# Makefile for TGL-6502
#----------------------------------------------------------------------------
# 16-Dec-2014 ShaneG
#
# Moving from LPCXpresso project to a make based build. Original Makefile
# from:
#        gcc Makefile for LPC810
#        Kamal Mostafa <kamal@whence.com>
#        License: Public Domain
#----------------------------------------------------------------------------

# Emulation target
#
# Can be specified on command line, currently only mos6502 is supported and
# is the default.
TARGET ?= mos6502

# Host processor
#
# Can be specified on command line, currently supported hosts include lpc810
# (the default) and local (for simulation)
HOST ?= lpc810

# Hardware revision
#
# Set the default hardware revision. Available hardware platforms are:
#  0 - Windows/Linux simulator
#  1 - LPC810 RevA
#  2 - LPC810 RevB
ifeq ($(HOST),lpc810)
HW_VERSION ?= 1
else
HW_VERSION ?= 0
endif

# Base filename
FILENAME = tgl6502

# Add flags to the C pre-processor
#
# We pass on the environment (target, host, hardware version) and set some
# common include directories
CFLAGS += -DSYS_TARGET=$(TARGET) -DSYS_HOST=$(HOST) -DHW_VER=$(HW_VERSION)
CFLAGS += -Iinclude -Ihardware/$(HOST) -Ihardware/common -Icpu/$(TARGET)

# Add flags for the linker
LDFLAGS += --gc-sections

# Set LPC810 platform specific settings
ifeq ($(HOST),lpc810)
CROSS = arm-none-eabi-
LINKERSCRIPT = $(FILENAME)_lpc810.ld
BUILD_TARGET = $(FILENAME).axf $(FILENAME).bin $(FILENAME).hex
# Adjust C command line appropriately
CFLAGS += -mthumb -mcpu=cortex-m0plus -MD
CFLAGS += -Os --specs=nano.specs -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -Iinclude/cmsis -D__USE_CMSIS
# Adjust LD command line appropriately
LDFLAGS += -T $(LINKERSCRIPT)
endif

# Set local platform specific settings
ifeq ($(HOST),local)
BUILD_TARGET = $(FILENAME)
endif

# Tools to use
CC = ${CROSS}gcc
LD = ${CROSS}ld
OBJCOPY = ${CROSS}objcopy
SIZE = ${CROSS}size

# Build the list of object files based on the selections
OBJECTS = main.o
OBJECTS += $(patsubst %.c,%.o,$(wildcard cpu/$(TARGET)/*.c))
OBJECTS += $(patsubst %.c,%.o,$(wildcard hardware/common/*.c))
OBJECTS += $(patsubst %.c,%.o,$(wildcard hardware/$(HOST)/*.c))

# And the list of dependencies
DEPENDS = $(patsubst %.o,%.d,$(OBJECTS))

# use these on the $(LD) line, as needed:
LIBM = ${shell ${CC} ${CFLAGS} --print-file-name=libm.a}
LIBC = ${shell ${CC} ${CFLAGS} --print-file-name=libc.a}
LIBGCC = ${shell ${CC} ${CFLAGS} --print-libgcc-file-name}

#----------------------------------------------------------------------------
# Build rules
#----------------------------------------------------------------------------

all: $(BUILD_TARGET)

# Rules to flash (depends on host processor)
ifeq ($(HOST),lpc810)
PORT ?= /dev/ttyUSB0
flash: $(BUILD_TARGET)
	lpc21isp $(FILENAME).hex $(PORT) 115200 14746
endif

# How to build the local binary
$(FILENAME): $(OBJECTS)
	@$(CC) -o $@ $(OBJECTS)

# How to build the AXF file
$(FILENAME).axf: $(LINKERSCRIPT) $(OBJECTS)
	@$(LD) -o $@ $(LDFLAGS) -Map $(FILENAME)_$(TARGET).map $(OBJECTS) "$(LIBGCC)"
	-@echo ""
	@$(SIZE) $(FILENAME).axf

# Generate a hex file
%.hex:%.axf
	-@echo "Generating $(FILENAME).hex (FlashMagic, etc.)"
	@$(OBJCOPY) --strip-unneeded -O ihex $(FILENAME).axf $(FILENAME).hex

# Generate a bin file
%.bin:%.axf
	-@echo ""
	-@echo "Generating $(FILENAME).bin"
	@$(OBJCOPY) --strip-unneeded -O binary $(FILENAME).axf $(FILENAME).bin

# Clean up
clean:
	@rm -f $(OBJECTS) $(DEPENDS) $(BUILD_TARGET)

