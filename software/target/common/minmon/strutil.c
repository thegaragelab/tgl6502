/*--------------------------------------------------------------------------*
* String utilities
*---------------------------------------------------------------------------*
* 22-Dec-2014 ShaneG
*
* Simple string utility functions (we don't use the C runtime library).
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "minmon.h"
#include "tgl6502.h"

/** Compare two strings
 *
 * Test two strings for equality.
 *
 * @param cszOne the first string.
 * @param cszTwo the second string.
 * @param length the number of characters to compare.
 *
 * @return true if the strings are equal up to the specified length.
 */
bool strncmp(const char *cszOne, const char *cszTwo, uint8_t length) {
  for(;length;length--,cszOne++,cszTwo++) {
    if(*cszOne!=*cszTwo)
      return false;
    }
  return true;
  }

/** Display a hex value
 *
 * Display a hexadecimal number (up to 16 bits) on the console.
 *
 * @param value the value to display in hex
 * @param digits the number of digits to use (1 to 4)
 */
void showHex(uint16_t value, int digits) {
  uint8_t current, offset = (digits - 1) * 4;
  uint16_t mask = 0x000f << offset;
  while(mask) {
    current = (value & mask) >> offset;
    if(current>=10)
      putch('A' + current - 10);
    else
      putch('0' + current);
    // Adjust mask and offset
    mask = mask >> 4;
    offset -= 4;
    }
  }

/** Convert a hex string into a value
 *
 * @param cszHex pointer to the string containing the hex value
 * @param length the number of characters to process
 *
 * @return the value represented by the hex string.
 */
uint16_t hexToWord(const char *cszHex, uint8_t length) {
  uint16_t value = 0;
  while(length--) {
    value = value << 4;
    if((*cszHex>='A')&&(*cszHex<='F'))
      value = value + (*cszHex - 'A' + 10);
    else if((*cszHex>='0')&&(*cszHex<='9'))
      value = value + (*cszHex - '0');
    }
  return value;
  }

