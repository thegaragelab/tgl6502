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
#include <stdlib.h>
#include <tgl6502.h>

#if defined(_MSC_VER)
#  include <conio.h>
#  include <Windows.h>
#else
#  include <termios.h>
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/time.h>
#endif

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
  // Check for overflow
  if(address>=sizeof(IO_STATE))
    return 0xFF;
  switch(address) {
    case IO_OFFSET_CONIN:
      return uartRead();
    case IO_OFFSET_IOCTL:
    case IO_OFFSET_SLOT0:
      // Read current IO
      if(uartAvail())
        g_ioState.m_ioctl |= IOCTL_CONDATA;
      else
        g_ioState.m_ioctl &= ~IOCTL_CONDATA;
      break;
    }
  return ((uint8_t *)&g_ioState)[address];
  }

/** Write a byte to the IO region
 *
 * @param address the offset into the IO area to write
 * @param value the value to write
 */
static void cpuWriteIO(uint16_t address, uint8_t byte) {
  // Check for overflow
  if(address>=sizeof(IO_STATE))
    return;
  switch(address) {
    case IO_OFFSET_VERSION:
    case IO_OFFSET_CONIN:
    case IO_OFFSET_KIPS:
    case IO_OFFSET_KIPS + 1:
      // Read only entries
      return;
    case IO_OFFSET_CONOUT:
      uartWrite(byte);
      return;
    }
  // Update the value
  ((uint8_t *)&g_ioState)[address] = byte;
  switch(address) {
    case IO_OFFSET_IOCTL:
    case IO_OFFSET_SLOT0:
      // Update IO state
      break;
    case IO_OFFSET_SPICMD:
      // Do the SPI transaction
      break;
    }
  }

//---------------------------------------------------------------------------
// Public API
//---------------------------------------------------------------------------

static uint32_t s_lastCheck = 0;

/** Determine if a quarter second has elapsed.
 *
 * @return true if 0.25 seconds has passed.
 */
bool qsecElapsed() {
  uint32_t now;
#if defined(_MSC_VER)
  SYSTEMTIME time;
  GetSystemTime(&time);
  now = (time.wSecond * 1000) + time.wMilliseconds;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  now = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
#endif // _MSC_VER
  if ((now - s_lastCheck)>250) {
    s_lastCheck = now;
    return true;
    }
  return false;
  }

/** Send a single character over the UART
 *
 * This is a blocking operation. If the transmitter is not yet ready the
 * function will block until it can send the character.
 *
 * @param ch the character to send.
 */
void uartWrite(uint8_t ch) {
  putchar(ch);
  fflush(stdout);
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
#if defined(_MSC_VER)
  return _getch();
#else
  // Translate LF to CR
  uint8_t ch = getchar();
  return (ch=='\n')?'\r':ch;
#endif
  }

/** Determine if data is available to read
 *
 * @return true if data is available and a call to 'uartRead()' would return
 *              immediately.
 */
bool uartAvail() {
#if defined(_MSC_VER)
  return _kbhit();
#else
  struct timeval tv;
  fd_set rdfs;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
#endif
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
#if !defined(_MSC_VER)
  // Change console mode
  struct termios oldt, newt;
  tcgetattr( STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt);
#endif
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
  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
    return cpuReadIO(address - IO_BASE);
  // Read from memory
  uint8_t result = 0;
  uint32_t phys = getPhysicalAddress(address);
  uint8_t *pMem = (phys&ROM_BASE)?g_ROM:g_RAM;
  phys &= 0x007FFFFL;
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
  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
    cpuWriteIO(address - IO_BASE, value);
  else {
    // Write data to RAM only
    uint32_t phys = getPhysicalAddress(address);
    if (phys>MEMORY_SIZE)
      return; // Out of RAM range
    g_RAM[phys] = value;
    }
  }

