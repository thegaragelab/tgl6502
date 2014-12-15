/*--------------------------------------------------------------------------*
* TGL-6502 Hardware Emulation
*---------------------------------------------------------------------------*
* 03-Dec-2014 ShaneG
*
* Provide a version of the hardware interface for use on the Windows test
* platform. It is incomplete (no slot support or general IO) but enough to
* test ROM code on.
*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <tgl6502.h>

//! Size of memory (using same size for ROM and RAM)
#define MEMORY_SIZE (128 * 1024)

//! Simulated RAM
uint8_t g_RAM[MEMORY_SIZE];

//! Simulated ROM
uint8_t g_ROM[MEMORY_SIZE];

//! IO state information
IO_STATE g_ioState;

//---------------------------------------------------------------------------
// Version information
//
// The version byte indicates the hardware platform in the upper nybble and
// the firmware version in the lower. Available platforms are:
//
//  0 - The desktop simulator.
//  1 - LPC810 based RevA hardware
//  2 - LPC810 based RevB hardware
//---------------------------------------------------------------------------

#define HW_VERSION 0
#define FW_VERSION 1

// The single version byte (readable from the IO block)
#define VERSION_BYTE ((HW_VERSION << 4) | FW_VERSION)

//---------------------------------------------------------------------------
// Helper functions
//---------------------------------------------------------------------------

/** Calculate the physical address with the current page settings.
 *
 * @param address the 16 bit CPU address
 *
 * @return the 32 bit (20 used) physical address.
 */
static uint32_t getPhysicalAddress(uint16_t address) {
  return ((uint32_t)address & 0x1FFF) | ((uint32_t)(g_ioState.m_pages[address >> 13] & 0x7F) << 13);
  }

/** Read a byte from the IO region
 *
 * @param address the offset into the IO area to read
 */
static uint8_t cpuReadIO(uint16_t address) {
  // TODO: Implement this
  }

/** Write a byte to the IO region
 *
 * @param address the offset into the IO area to write
 * @param value the value to write
 */
static void cpuWriteIO(uint16_t address, uint8_t byte) {
  // TODO: Implement this
  }

//---------------------------------------------------------------------------
// Public API
//---------------------------------------------------------------------------

/** Send a single character over the UART
 *
 * This is a blocking operation. If the transmitter is not yet ready the
 * function will block until it can send the character.
 *
 * @param ch the character to send.
 */
void uartWrite(uint8_t ch) {
  printf("%c", ch);
  }

/** Send a sequence of bytes to the UART
 *
 * Sends the contents of a NUL terminated string over the UART interface.
 */
void uartWriteString(const char *cszString) {
  printf("%s", cszString);
  }

/** Read a single character from the UART
 *
 * This is a blocking operation. If no data is available the function will
 * block until it is. Use the 'uartRead()' function to determine if data is
 * available.
 *
 * @return the character read from the UART
 */
uint8_t uartRead() {
  return _getch();
  }

/** Determine if data is available to read
 *
 * @return true if data is available and a call to 'uartRead()' would return
 *              immediately.
 */
bool uartAvail() {
  return _kbhit();
  }

/** Initialise the external hardware
 *
 * In the windows implementation we simply load the ROM file (6502.rom) into
 * memory.
 */
void hwInit() {
  // Load the contents of ROM
  FILE *fp = fopen("6502.rom", "rb");
  if (fp == NULL) {
    printf("ERROR: Can not load ROM file '6502.rom'\n");
    exit(1);
    }
  uint16_t read, index = 0;
  do {
    read = fread(&g_ROM[index], 1, MEMORY_SIZE - index, fp);
    index += read;
    }
  while (read > 0);
  fclose(fp);
  printf("Read %u bytes from '6502.rom'\n", index);
  }

/** Get the current timestamp in milliseconds
 *
 * This function is used for basic duration timing. The base of the timestamp
 * doesn't matter as long as the result increments every millisecond.
 *
 * @return timestamp in milliseconds
 */
uint32_t getMillis() {
  SYSTEMTIME time;
  GetSystemTime(&time);
  return (time.wSecond * 1000) + time.wMilliseconds;
  }

/** Determine the duration (in ms) since the last sample
 *
 * @param sample the last millisecond count
 */
uint32_t getDuration(uint32_t sample) {
  uint32_t now = getMillis();
  if(sample > now)
    return now + (0xFFFFFFFFL - sample);
  return now - sample;
  }

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
  // Stash the version information
  g_ioState.m_version = VERSION_BYTE;
  }

/** Read a single byte from the CPU address space.
*
* @param address the 16 bit address to read a value from.
*
* @return the byte read from the address.
*/
uint8_t cpuReadByte(uint16_t address) {
  // Check for IO reads
//  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
//    return cpuReadIO(address - IO_BASE);
  if (address==0xF004)
    return uartAvail()?uartRead():0;
  // Read from memory
  uint8_t result = 0;
  uint32_t phys = getPhysicalAddress(address);
  uint8_t *pMem = (phys&ROM_BASE)?g_ROM:g_RAM;
  phys &= 0x0007FFFL;
  if (phys<MEMORY_SIZE)
    result = pMem[phys];
  return result;
  }

/** Write a single byte to the CPU address space.
*
* @param address the 16 bit address to write a value to
*/
void cpuWriteByte(uint16_t address, uint8_t value) {
  // Check for IO writes
//  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
//    cpuWriteIO(address - IO_BASE, value);
  if (address == 0xF001)
    uartWrite(value);
  else {
    // Write data to RAM only
    uint32_t phys = getPhysicalAddress(address);
    if (phys>MEMORY_SIZE)
      return; // Out of RAM range
    g_RAM[phys] = value;
    }
  }

