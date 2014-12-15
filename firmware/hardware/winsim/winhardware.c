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
#include "cpu6502.h"
#include "hardware.h"

//! Simulated RAM
uint8_t g_RAM[MEMORY_SIZE];

//! Simulated ROM
uint8_t g_ROM[MEMORY_SIZE];

//! IO state information
IO_STATE g_ioState;

/** Initialise the UART interface
 *
 * Configures the UART for 57600 baud, 8N1.
 */
void uartInit() {
  }

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

/** Initialise SPI interface
 *
 * Set up SPI0 as master in mode 0 (CPHA and CPOL = 0) at 10MHz with no delays.
 * SSEL is not assigned to an output pin, it must be controlled separately.
 */
void spiInit() {
  }

/** Transfer a single data byte via SPI
 *
 * @param data the 8 bit value to send
 *
 * @return the data received in the transfer
 */
uint8_t spiTransfer(uint8_t data) {
  // NOTE: Not implemented for Windows test harness
  return 0xFF;
  }

/** Initialise the external hardware
 *
 * This function needs to be called after the SPI bus has been initialised and
 * the pin switch matrix has been set up. It will initialise the external SPI
 * components.
 */
void hwInit() {
  }

/** Read a page of data from the EEPROM
 *
 * Fill a buffer with data from the selected page. The buffer must be large
 * enough to hold the page (@see EEPROM_PAGE_SIZE).
 *
 * @param address the page address to read
 * @param pData pointer to the buffer to hold the data.
 */
void eepromReadPage(uint16_t address, uint8_t *pData) {
  // NOTE: Not implemented for Windows test harness
  }

/** Write a page of data to the EEPROM
 *
 * Write a buffer of data to the selected page. The buffer must be large
 * enough to hold the page (@see EEPROM_PAGE_SIZE).
 *
 * @param address the page address to write
 * @param pData pointer to the buffer holding the data.
 */
void eepromWritePage(uint16_t address, uint8_t *pData) {
  // NOTE: Not implemented for Windows test harness
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
  }

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
  uint8_t *pData = (uint8_t *)g_ioState;
  switch(address) {
  case IO_OFFSET_CONIN:
    break;

  return pData[address];
  }

/** Write a byte to the IO region
 *
 * @param address the offset into the IO area to write
 * @param value the value to write
 */
static void cpuWriteIO(uint16_t address, uint8_t byte) {
  // TODO: Implement this
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

