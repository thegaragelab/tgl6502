/*---------------------------------------------------------------------------*
* Microboard string formatting support
*----------------------------------------------------------------------------*
* 19-Nov-2014 ShaneG
*
* Implement the portable string formatting functions.
*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "tgl6502.h"
#include "tglstring.h"

//---------------------------------------------------------------------------
// Public API
//---------------------------------------------------------------------------

/** Print a string
 *
 * This function simply prints the nul terminated string.
 *
 * @param cszString pointer to a character array in RAM.
 */
void strPrint(const char *cszString) {
  for(;*cszString; cszString++)
    putch(*cszString);
  }

/** Write a fixed length string to the console
 *
 * @param str the string to write.
 * @param len the number of character to write.
 *
 * @return the number of characters written.
 */
int strPrintL(const char *str, uint8_t len) {
  uint8_t index;
  for(index=0; (index<len) && str[index]; index++)
    putch(str[index]);
  return index;
  }

/** Print an unsigned 16 bit value in decimal
 *
 * Print the given value in decimal format.
 *
 * @param value the value to print.
 */
void strPrintInt(uint16_t value) {
  uint16_t divisor;
  bool emit = false;
  // Special case for 0
  if(value==0) {
    putch('0');
    return;
    }
  // Emit the value, skip leading zeros
  for(divisor = 10000; divisor > 0; divisor = divisor / 10) {
    uint8_t digit = value / divisor;
    value = value % divisor;
    if((digit>0)||emit) {
      putch('0' + digit);
      emit = true;
      }
    }
  }

/** Print an unsigned 16 bit value in hexadecimal
 *
 * Print the given value in hexadecimal format.
 *
 * @param value the value to print.
 * @param digits the number of digits to use for display.
 */
void strPrintHex(uint16_t value, uint8_t digits) {
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

/** Print a formatted string
 *
 * Prints a formatted string. This function supports a subset of the 'printf'
 * string formatting syntax. Allowable insertion types are:
 *
 *  %% - Display a % character. There should be no entry in the variable
 *       argument list for this entry.
 *  %u - Display an unsigned integer in decimal. The matching argument may
 *       be any 16 bit value.
 *  %x - Display an unsigned byte in hexadecimal. The matching argument may
 *       be any 8 bit value.
 *  %X - Display an unsigned int in hexadecimal. The matching argument may
 *       by any 16 bit value.
 *  %c - Display a single ASCII character. The matching argument may be any 8
 *       bit value.
 *  %s - Display a NUL terminated string. The matching argument must be a
 *       pointer to a NUL terminated string.
 *
 * @param cszString pointer to a nul terminated format string in RAM.
 */
void strFormat(const char *cszString, ...) {
  va_list args;
  int index;
  bool skip;
  char ch1, ch2 = *cszString;
  va_start(args, cszString);
  for(index=1; ch2!='\0'; index++) {
    ch1 = ch2;
    ch2 = cszString[index];
    skip = true;
    if(ch1=='%') {
      // Use the second character to determine what is requested
      if((ch2=='%')||(ch2=='\0'))
        putch('%');
      else if(ch2=='c')
        putch(va_arg(args, int));
      else if(ch2=='u')
        strPrintInt(va_arg(args, uint16_t));
      else if(ch2=='x')
        strPrintHex(va_arg(args, uint8_t), 2);
      else if(ch2=='X')
        strPrintHex(va_arg(args, uint16_t), 4);
      else if(ch2=='s')
        strPrint(va_arg(args, char *));
      }
    else {
      putch(ch1);
      skip = false;
      }
    if(skip) {
      // Move ahead an extra character so we wind up jumping by two
      ch1 = ch2;
      ch2 = cszString[++index];
      }
    }
  va_end(args);
  }

