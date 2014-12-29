/*---------------------------------------------------------------------------*
* Common interface to the TGL-6502 hardware
*----------------------------------------------------------------------------*
* 20-Dec-2014 ShaneG
*
* Provides data structures and constants for interacting with the TGL-6502
* memory mapped hardware. Also provides a handful of basic helper functions
* to manipulate the hardware.
*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "tgl6502.h"

/** The memory mapped IO region */
TGL6502_IO *pIO = (TGL6502_IO *)IO_BASE;

/** Get a single character from the console
 *
 * This function blocks until a character is ready and then returns the ASCII
 * code for that character. You can use the 'kbhit()' function to determine if
 * a character is ready to be read.
 *
 * @return the ASCII value of the character pressed.
 */
uint8_t getch() {
  // Wait for input to become available
  while(!(pIO->m_ioctl&IOCTL_CONDATA));
  return pIO->m_conin;
  }

/** Determine if a character is available to be read.
 *
 * This function can be used to implement non-blocking character IO without
 * interrupts. If it returns true the 'getch()' function will immediately
 * return a character without blocking.
 *
 * @return true if a character is available to be read.
 */
bool kbhit() {
  return pIO->m_ioctl&IOCTL_CONDATA;
  }

/** Write a single character to the console
 *
 * @param ch the character to write
 */
void putch(uint8_t ch) {
  pIO->m_conout = ch;
  }

  /** Write a NUL terminated string to the console
 *
 * @param str the string to write.
 *
 * @return the number of characters written.
 */
int puts(const char *str) {
  int index = 0;
  while(str[index])
    putch((uint8_t)str[index++]);
  return index;
  }

