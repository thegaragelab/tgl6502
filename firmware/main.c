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
#include <Windows.h>
#include "cpu6502.h"

//! Size of ROM in emulated machine
#define ROM_SIZE (128 * 1024)

//! The memory allocated for the ROM
extern uint8_t g_ROM[];

/** Read a character from the keyboard
 *
 * Return 0 if no character is available otherwise return the ASCII character.
 */
uint8_t readChar() {
  if (_kbhit())
    return _getch();
  return 0;
  }

/** Write a character to the console.
 */
void writeChar(uint8_t ch) {
  printf("%c", ch);
  }

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
  uint16_t inner = 0, outer = 0;
  while (true) {
    cpuStep();
    inner++;
    if (inner==500) {
      Sleep(1);
      inner = 0;
      outer++;
      if (outer == 500) {
        fprintf(fp, "%u,%u,%u,%u,%u,%u\n",
          g_memoryStats.m_reads,
          g_memoryStats.m_writes,
          g_memoryStats.m_hit,
          g_memoryStats.m_miss,
          g_memoryStats.m_loads,
          g_memoryStats.m_saves
          );
        fflush(fp);
        outer = 0;
        g_memoryStats.m_reads = 0;
        g_memoryStats.m_writes = 0;
        g_memoryStats.m_hit = 0;
        g_memoryStats.m_miss = 0;
        g_memoryStats.m_loads = 0;
        g_memoryStats.m_saves = 0;
        }
      }
    }
  return 0;
  }

