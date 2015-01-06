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
  uint8_t index;
  for(index=0;index<length;index++)
    if(cszOne[index]!=cszTwo[index])
      return false;
  return true;
  }

/** Convert a hex string into a value
 *
 * @param cszHex pointer to the string containing the hex value
 * @param length the number of characters to process
 *
 * @return the value represented by the hex string.
 */
uint16_t hexToWord(const char *cszHex, uint8_t length) {
  uint8_t index;
  uint16_t value = 0;
  for(index=0;index<length;index++) {
    value = value << 4;
    if((cszHex[index]>='A')&&(cszHex[index]<='F'))
      value = value + (cszHex[index] - 'A' + 10);
    else if((cszHex[index]>='0')&&(cszHex[index]<='9'))
      value = value + (cszHex[index] - '0');
    }
  return value;
  }

