# The TGL-6502 Single Board Computer

This project implements 6502 emulator running in 4K of flash on an LPC810
microcontroller. For more information please visit [this site](http://thegaragelab.com/posts/introducing-the-tgl-6502-single-board-computer.html).

## Hardware

The hardware is very simple - the CPU itself, an SPI IO expander, two SPI memory
chips and some discrete logic. Full schematics and PCB layouts have been provided
in the *hardware* directory. The current board revision is RevB, all future work
will be using that hardware.

The hardware design is provide in [DesignSpark PCB](http://www.rs-online.com/designspark/electronics/eng/page/designspark-pcb-home-page)
format - this is a free, fully featured schematic capture and PCB layout tool
Additional formats (PDF and gerber files) have been provided in the *hardware/RevB/mfr*
directory if you prefer to use a different tool.

## Firmware

The firmware consists of a [LPCXpresso](http://www.lpcware.com/lpcxpresso) project
but is in the process of migrating to a *makefile* based build system.

## Licensing

The project is being released under the [Creative Commons Attribution-ShareAlike](http://creativecommons.org/licenses/by-sa/4.0/)
license. Some components of the project are sourced from others - their licenses
are included in their source directories.

