/*--------------------------------------------------------------------------*
* 6502 CPU Emulator - Test Harness
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This is a simple Windows command line application to test the emulator.
*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cpu6502.h"

#define CONSOLE_OUT 0xF001
#define CONSOLE_IN  0xF004

//---------------------------------------------------------------------------
// Simple memory management
//---------------------------------------------------------------------------

//! Size of RAM in emulated machine
#define RAM_SIZE (48 * 1024)

//! Size of ROM in emulated machine
#define ROM_SIZE (16 * 1024)

//! RAM memory
static uint8_t g_RAM[RAM_SIZE];

//! ROM memory
static uint8_t g_ROM[ROM_SIZE];

/** Read a single byte from the CPU address space.
*
* @param address the 16 bit address to read a value from.
*
* @return the byte read from the address.
*/
uint8_t cpuReadByte(uint16_t address) {
  // Simulated keyboard for EhBASIC
  if (address == CONSOLE_IN) {
	if (_kbhit())
	  return _getch();
	return 0;
    }
  // Read from memory
  if(address < RAM_SIZE)
	return g_RAM[address];
  return g_ROM[address - RAM_SIZE];
  }

/** Write a single byte to the CPU address space.
*
* @param address the 16 bit address to write a value to
*/
void cpuWriteByte(uint16_t address, uint8_t value) {
  // Simulate console for EhBASIC
  if(address == CONSOLE_OUT) {
	printf("%c", value);
	return;
	}
  // Write data to RAM only
  if(address < RAM_SIZE)
	g_RAM[address] = value;
  else // Whoops, writing to ROM
    printf("CPU: Attempt to write to ROM - %04x = %02x\n", address, value);
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
  // Now run the emulator
  cpuReset();
  while (true)
	cpuStep();
  return 0;
  }
