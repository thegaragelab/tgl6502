/*--------------------------------------------------------------------------*
* Global functions and constants for MinMon
*---------------------------------------------------------------------------*
* 22-Dec-2014 ShaneG
*
* Defines common constants and functions for the MinMon monitor
*--------------------------------------------------------------------------*/
#ifndef __MINMON_H
#define __MINMON_H

//---------------------------------------------------------------------------
// Helper functions
//---------------------------------------------------------------------------

/** Determine if a character is a valid hexadecimal digit
 *
 * @param ch the character to test (ASCII)
 *
 * @return true if the value is a hexadecimal digit.
 */
#define ishex(ch) ((((ch)>='A')&&((ch)<='F'))||(((ch)>='0')&&((ch)<='9')))

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
bool strncmp(const char *cszOne, const char *cszTwo, uint8_t length);

/** Display a hex value
 *
 * Display a hexadecimal number (up to 16 bits) on the console.
 *
 * @param value the value to display in hex
 * @param digits the number of digits to use (1 to 4)
 */
void showHex(uint16_t value, int digits);

/** Convert a hex string into a value
 *
 * @param cszHex pointer to the string containing the hex value
 * @param length the number of characters to process
 *
 * @return the value represented by the hex string.
 */
uint16_t hexToWord(const char *cszHex, uint8_t length);

//---------------------------------------------------------------------------
// Command definitions and lookup function
//---------------------------------------------------------------------------

/** Available commands
 */
typedef enum {
  CMD_ADDRESS = 0, //!< Set the working address
  CMD_READ,        //!< Read data from memory
  CMD_WRITE,       //!< Write data to memory
  CMD_GOTO,        //!< Execute code
  // This must always be last
  CMD_INVALID      //!< Invalid command
  } MINMON_CMD;

/** Get the command ID from a string
 *
 * Convert a string into a command ID.
 *
 * @param cszCmd pointer to the string containing the command name.
 * @param length the length of the string in characters
 *
 * @return the command ID or CMD_INVALID if the string does not represent a
 *         valid command.
 */
MINMON_CMD cmdLookup(const char *cszCmd, uint8_t length);

#endif /* __MINMON_H */

