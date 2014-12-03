/*--------------------------------------------------------------------------*
* 6502 CPU Emulator - Test Harness
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This is a simple Windows command line application to test the emulator.
*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cpu6502.h"
#include "hardware.h"

//! The memory allocated for the ROM
extern uint8_t g_ROM[];

/** Program entry point
 */
int main(int argc, char *argv[]) {
  // Load the contents of ROM
  FILE *fp = fopen("6502.rom", "rb");
  if (fp == NULL) {
    printf("ERROR: Can not load ROM file '6502.rom'\n");
    return 1;
    }
  uint16_t read, index = 0;
  do {
    read = fread(&g_ROM[index], 1, MEMORY_SIZE - index, fp);
    index += read;
    }
  while (read > 0);
  fclose(fp);
  printf("Read %u bytes from '6502.rom'\n", index);
  // Now run the emulator
  cpuResetIO();
  cpuReset();
  while (true)
    cpuStep();
  return 0;
  }

