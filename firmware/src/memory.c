/*--------------------------------------------------------------------------*
* 6502 CPU Emulator - Memory management
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This file implements the memory and IO access functions for the emulator.
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "cpu6502.h"

/* These are the addresses used for console input and output by EhBASIC
 *
 * Eventually these will get replaced with a more integrated IO emulation and
 * redirect to the serial port on the host MCU. For now they are hardcoded.
 */
#define CONSOLE_OUT 0xF001
#define CONSOLE_IN  0xF004

//! Size of RAM in emulated machine
#define RAM_SIZE (128 * 1024)

//! Size of ROM in emulated machine
#define ROM_SIZE (128 * 1024)

//! The start of ROM space in physical memory
#define ROM_BASE 0x00080000L

//! The base CPU address for the IO page
#define IO_BASE 0xFF00

#if defined(_MSC_VER) // For Windows simulator
//! RAM memory
uint8_t g_RAM[RAM_SIZE];

//! ROM memory
uint8_t g_ROM[ROM_SIZE];
#endif

//! IO state information
IO_STATE g_ioState;

/** Initialise the memory and IO subsystem
 *
 * This function should be called every time the 'cpuReset()' function is
 * called. In ensures the memory mapped IO region is simulating the 'power on'
 * state.
 */
void cpuResetIO() {
  uint8_t index, *pIO = (uint8_t *)&g_ioState;
  // Clear the entire IO space
  for(index=0; index<sizeof(IO_STATE); index++)
    pIO[index] = 0;
  // Set up the default memory mapping
  for(index=0; index<6; index++)
    g_ioState.m_pages[index] = index;
  g_ioState.m_pages[6] = 0x40; // ROM page 0
  g_ioState.m_pages[7] = 0x41; // ROM page 1
  }

/** Calculate the physical address with the current page settings.
 *
 * @param address the 16 bit CPU address
 *
 * @return the 32 bit (20 used) physical address.
 */
uint32_t getPhysicalAddress(uint16_t address) {
  return ((uint32_t)address & 0x1FFF) | ((uint32_t)(g_ioState.m_pages[address >> 13] & 0x7F) << 13);
  }

/** Read a single byte from the CPU address space.
 *
 * @param address the 16 bit address to read a value from.
 *
 * @return the byte read from the address.
 */
uint8_t cpuReadByte(uint16_t address) {
#if defined(_MSC_VER) // For Windows simulator
  // Simulated keyboard for EhBASIC
  if (address == CONSOLE_IN)
    return readChar();
#endif
  // Check for IO reads
  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
    return cpuReadIO(address - IO_BASE);
  // Read from memory
  uint32_t phys = getPhysicalAddress(address);
#if defined(_MSC_VER) // For Windows simulator
  if (phys&ROM_BASE)
    return (phys<(ROM_BASE + ROM_SIZE))?g_ROM[phys & ~ROM_BASE]:0;
  // It's in RAM
  return (phys<RAM_SIZE)?g_RAM[phys]:0;
#else
  return 0;
#endif
  }

/** Write a single byte to the CPU address space.
 *
 * @param address the 16 bit address to write a value to
 */
void cpuWriteByte(uint16_t address, uint8_t value) {
#if defined(_MSC_VER) // For Windows simulator
  // Simulate console for EhBASIC
  if(address == CONSOLE_OUT) {
    writeChar(value);
    return;
    }
#endif
  // Check for IO writes
  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
    cpuWriteIO(address - IO_BASE, value);
  else {
    // Write data to RAM only
    uint32_t phys = getPhysicalAddress(address);
#if defined(_MSC_VER) // For Windows simulator
    if (phys<RAM_SIZE)
      g_RAM[phys] = value;
#endif
    }
  }
