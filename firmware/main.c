/*--------------------------------------------------------------------------*
* 6502 CPU Emulator - Test Harness
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This is a simple Windows command line application to test the emulator.
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <tgl6502.h>

/** Program entry point
 */
int main(int argc, char *argv[]) {
  // Initialise the hardware
  hwInit();
  // And run the emulator
  cpuResetIO();
  cpuReset();
  while (true)
    cpuStep();
  return 0;
  }

