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

FILENAME = tgl6502
LINKERSCRIPT = tgl6502.ld

INC = -Iinclude -Ihardware/$(HOST) -Ihardware/common -Icpu/$(TARGET)

# Set platform specific
ifeq ($(HOST),lpc810)
CROSS = arm-none-eabi-
CPU = -mthumb -mcpu=cortex-m0plus -MD
FPU = # -mfloat-abi=soft
CFLAGS+= -Os --specs=nano.specs -ffunction-sections -fdata-sections -fno-builtin
INC += -Iinclude/cmsis
endif

# Tools to use
CC = ${CROSS}gcc
LD = ${CROSS}ld
OBJCOPY = ${CROSS}objcopy
SIZE = ${CROSS}size

# use these on the $(LD) line, as needed:
LIBM = ${shell ${CC} ${CFLAGS} --print-file-name=libm.a}
LIBC = ${shell ${CC} ${CFLAGS} --print-file-name=libc.a}
LIBGCC = ${shell ${CC} ${CFLAGS} --print-libgcc-file-name}

CFLAGS += $(CPU) $(FPU)
LDFLAGS += --gc-sections

CFLAGS += $(INC) -D__USE_CMSIS # -DCLOCK_SETUP=1

OBJECTS = main.o
OBJECTS += $(patsubst %.c,%.o,$(wildcard cpu/$(TARGET)/*.c))
OBJECTS += $(patsubst %.c,%.o,$(wildcard hardware/common/*.c))
OBJECTS += $(patsubst %.c,%.o,$(wildcard hardware/$(HOST)/*.c))

all: $(FILENAME).bin

$(FILENAME).axf: $(FILENAME).ld $(OBJECTS)
	@$(LD) -o $@ $(LDFLAGS) -Map $(FILENAME).map -T $(LINKERSCRIPT) $(OBJECTS) "$(LIBGCC)"
	-@echo ""
	@$(SIZE) $(FILENAME).axf

clean:
	@rm -f *.o *.d
	@rm -f $(FILENAME).axf $(FILENAME).hex $(FILENAME).bin

%.bin:%.axf
	-@echo ""
	-@echo "Generating $(FILENAME).hex (FlashMagic, etc.)"
	@$(OBJCOPY) --strip-unneeded -O ihex $(FILENAME).axf $(FILENAME).hex
	-@echo "Generating $(FILENAME).bin"
	@$(OBJCOPY) --strip-unneeded -O binary $(FILENAME).axf $(FILENAME).bin
