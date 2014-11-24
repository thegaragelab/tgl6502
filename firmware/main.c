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

//! Size of ROM in emulated machine
#define ROM_SIZE (128 * 1024)

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
    read = fread(&g_ROM[index], 1, ROM_SIZE - index, fp);
    index += read;
    }
  while (read > 0);
  fclose(fp);
  printf("Read %u bytes from '6502.rom'\n", index);
  // Set up the statistics log
  fp = fopen("memory.log", "w");
  // Now run the emulator
  cpuResetIO();
  cpuReset();
  uint16_t count = 0;
  while (true) {
    cpuStep();
    count++;
    if (count==1000) {
      fprintf(fp, "%ul,%ul,%ul,%ul\n",
        g_memoryStats.m_reads,
        g_memoryStats.m_writes,
        g_memoryStats.m_loads,
        g_memoryStats.m_saves
        );
      fflush(fp);
      count = 0;
      g_memoryStats.m_reads = 0;
      g_memoryStats.m_writes = 0;
      g_memoryStats.m_loads = 0;
      g_memoryStats.m_saves = 0;
      }
    }
  return 0;
  }
