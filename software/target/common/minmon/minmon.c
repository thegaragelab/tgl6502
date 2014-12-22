/*---------------------------------------------------------------------------*
* Minimal Monitor (MinMon)
*----------------------------------------------------------------------------*
* 20-Dec-2014 ShaneG
*
* Provides simple low-level access to the memory and IO on the target.
*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "tgl6502.h"

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

//! Current working address
uint16_t g_addr = 0x0400;

//! Current input line
char g_line[LINE_LENGTH];

//! Line length
uint8_t g_length;

/** Display a hex value
 */
static void showHex(uint16_t value, int digits) {
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

/** Read a line of input
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

/** Program entry point
 *
 */
void main() {
  puts(BANNER "\n");
  // TODO: Copy ourselves to RAM and swap pages
  // Go into main processing loop
  while(true) {
    // Show the prompt
    putch('$');
    showHex(g_addr, 4);
    putch('>');
    // Read the next line
    readLine();
    // TODO: Read commands and process
    }
  }

