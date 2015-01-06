/*---------------------------------------------------------------------------*
* Minimal Monitor (MinMon)
*----------------------------------------------------------------------------*
* 20-Dec-2014 ShaneG
*
* Provides simple low-level access to the memory and IO on the target.
*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <tgl6502.h>
#include <tglstring.h>
#include "minmon.h"

//! Banner to display
#define BANNER "MinMon V1.0"

//! Backspace character
#define BS 0x08

//! Carriage return character
#define CR 0x0d

//! Line feed character
#define LF 0x0a

//! Maximum line length
#define LINE_LENGTH 80

/* Globals
 */
uint16_t g_addr = 0x0400;     //!< Current working address
char     g_line[LINE_LENGTH]; //!< Current input line
uint8_t  g_length;            //!< Line length
uint8_t  g_index;             //!< Index into line for token processing
uint16_t g_value;             //!< Value of last token discovered

/** Read a line of input
 *
 * Reads a line of input into the global 'g_line' variable and sets the line
 * length in 'g_length'. This function performs basic line editing (backspace
 * and character echo) and limits the allowed input characters.
 */
static void readLine() {
  uint8_t ch;
  g_length = 0;
  while(g_length<LINE_LENGTH) {
    ch = getch();
    // Check for safe characters and make them upper case
    if((ch>='a')&&(ch<='z'))
      ch = ch - 'a' + 'A';
    if(!(((ch>='A')&&(ch<='Z'))||((ch>='0')&&(ch<='9'))||(ch=='$')||(ch==' ')||(ch==BS)||(ch==CR))) {
      // TODO: Indicate an error
      continue;
      }
    // Process special characters
    if(ch==CR) {
      g_line[g_length] = '\0';
      putch(ch);
      putch(LF);
      return;
      }
    else if(ch==BS) {
      if(g_length>0) {
        g_length--;
        putch(ch);
        putch(' ');
        putch(ch);
        }
      else {
        // TODO: Indicate an error
        }
      }
    else if(g_length==(LINE_LENGTH - 1)) {
      // TODO: Indicate an error
      }
    else {
      g_line[g_length++] = ch;
      putch(ch);
      }
    }
  }

typedef enum {
  TOKEN_CMD,   //!< A command string
  TOKEN_BYTE,  //!< A two digit byte
  TOKEN_WORD,  //!< A four digit word
  TOKEN_EOL,   //!< End of input line
  TOKEN_ERROR, //!< Unrecognised token
  } TOKEN;

static TOKEN getToken() {
  uint8_t length;
  const char *cszStart;
  // Skip spaces
  while((g_line[g_index]==' ')&&(g_index<g_length))
    ++g_index;
  // Check for EOL
  if(g_index>=g_length)
    return TOKEN_EOL;
  // Check for hex constants
  if(g_line[g_index]=='$') {
    cszStart = &g_line[++g_index];
    length = 0;
    while((g_index<g_length)&&ishex(g_line[g_index])) {
      ++g_index;
      ++length;
      }
    g_value = hexToWord(cszStart, length);
    if(length==2)
      return TOKEN_BYTE;
    if(length==4)
      return TOKEN_WORD;
    return TOKEN_ERROR;
    }
  // Check for commands
  if((g_line[g_index]>='A')&&(g_line[g_index]<='Z')) {
    cszStart = &g_line[g_index];
    length = 0;
    while((g_index<g_length)&&((g_line[g_index]>='A')&&(g_line[g_index]<='Z'))) {
      ++g_index;
      ++length;
      }
    g_value = cmdLookup(cszStart, length);
    return ((g_value)==CMD_INVALID)?TOKEN_ERROR:TOKEN_CMD;
    }
  // Invalid input
  return TOKEN_ERROR;
  }

/** Program entry point
 *
 */
void main() {
  bool incaddr;
  MINMON_CMD cmd;
  uint16_t addr;
  uint8_t index;
  TOKEN token;
  // Do some identification
  strPrint(BANNER "\r\n");
  // TODO: Copy ourselves to RAM and swap pages
  // Go into main processing loop
  while(true) {
    // Show the prompt
    putch('$');
    strPrintHex(g_addr, 4);
    putch('>');
    // Read the next line
    readLine();
    incaddr = false;
    g_index = 0;
    // Get the first token
    token = getToken();
    // Check for command and default to 'READ'
    if(token==TOKEN_CMD) {
      cmd = g_value;
      token = getToken();
      }
    else
      cmd = CMD_READ;
    // Check for an address and default to current address
    if(token==TOKEN_WORD) {
      addr = g_value;
      token = getToken();
      }
    else {
      addr = g_addr;
      incaddr = true;
      }
    // Check for errors
    if(token==TOKEN_ERROR) {
      strPrint("ERROR: Unrecognised command.\r\n");
      continue;
      }
    // Process command
    switch(cmd) {
      case CMD_READ:
        // Read 16 bytes and display them
        putch('$');
        strPrintHex(addr, 4);
        putch(':');
        for(index=0;index<16;index++,addr++) {
          strPrint(" $");
          strPrintHex(*((uint8_t *)addr), 2);
          }
        strPrint("\r\n");
        break;
      case CMD_ADDRESS:
        incaddr = true;
        break;
      case CMD_WRITE:
        // Write bytes to memory
        while(token==TOKEN_BYTE) {
          *((uint8_t *)addr) = (uint8_t)(g_value & 0xFF);
          addr++;
          token = getToken();
          }
        break;
      default:
        strPrint("ERROR: Command not implemented.\r\n");
        continue;
      }
    // Be neat
    if(token!=TOKEN_EOL)
      strPrint("WARNING: Additional content at end of line was ignored.\r\n");
    // Update current address if required
    if(incaddr)
      g_addr = addr;
    }
  }

