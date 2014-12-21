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

#define BANNER "MinMon V1.0"

/** Program entry point
 *
 */
void main() {
  puts(BANNER "\n");
  // TODO: Copy ourselves to RAM and swap pages
  // Go into main processing loop
  while(true) {
    // TODO: Read commands and process
    }
  }

