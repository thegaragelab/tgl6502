/*---------------------------------------------------------------------------*
* Simple string formatting support
*----------------------------------------------------------------------------*
* 19-Nov-2014 ShaneG
*
* Provide simple string formatting functions. Adapted from the Microboard
* runtime library implementation.
*---------------------------------------------------------------------------*/
#ifndef __TGLSTRING_H
#define __TGLSTRING_H

//--- Bring in required definitions
#include <stdint.h>
#include <stdbool.h>

//---------------------------------------------------------------------------
// Basic output for data types.
//---------------------------------------------------------------------------

/** Print a string
 *
 * This function simply prints the nul terminated string.
 *
 * @param cszString pointer to a character array in RAM.
 */
void strPrint(const char *cszString);

/** Write a fixed length string to the console
 *
 * @param str the string to write.
 * @param len the number of character to write.
 *
 * @return the number of characters written.
 */
int strPrintL(const char *str, uint8_t len);

/** Print an unsigned 16 bit value in decimal
 *
 * Print the given value in decimal format.
 *
 * @param value the value to print.
 */
void strPrintInt(uint16_t value);

/** Print an unsigned 16 bit value in hexadecimal
 *
 * Print the given value in hexadecimal format.
 *
 * @param value the value to print.
 * @param digits the number of digits to use for display.
 */
void strPrintHex(uint16_t value, uint8_t digits);

//---------------------------------------------------------------------------
// Formatted output
//---------------------------------------------------------------------------

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
void strFormat(const char *cszString, ...);

#endif /* __TGLSTRING_H */

