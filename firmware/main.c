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

#define PARA_COUNT 4
#define PARA_SIZE  128

#define PARA_SEQUENCE 0x03
#define PARA_DIRTY    0x04
#define PARA_ADDRESS  0xFFF8
#define PARA_MASK     0xFFFC
#define PARA_ROM      0x8000

typedef struct _PARA_TABLE {
  uint16_t m_paraInfo[PARA_COUNT]; //!< Information about loaded paragraphs
  uint8_t  m_paraData[PARA_COUNT * PARA_SIZE]; //!< Cached paragraphs
  } PARA_TABLE;

static PARA_TABLE g_paragraphs;

//! Size of RAM in emulated machine
#define RAM_SIZE (128 * 1024)

//! Size of ROM in emulated machine
#define ROM_SIZE (128 * 1024)

//! RAM memory
static uint8_t g_RAM[RAM_SIZE];

//! ROM memory
static uint8_t g_ROM[ROM_SIZE];

//! IO state information
IO_STATE g_ioState;

/** Initialise the memory and IO subsystem
*
* This function should be called every time the 'cpuReset()' function is
* called. In ensures the memory mapped IO region is simulating the 'power on'
* state.
*/
void cpuResetIO() {
  uint8_t index;
  // Set up the default memory mapping
  for(index=0; index<6; index++)
    g_ioState.m_pages[index] = index;
  g_ioState.m_pages[6] = 0x40; // ROM page 0
  g_ioState.m_pages[7] = 0x41; // ROM page 1
  // Initialise the paragraph table (use first 4 ram paragraphs)
  for(index=0; index<PARA_COUNT; index++)
    g_paragraphs.m_paraInfo[index] = (index << 3) | index;
  }

/** Load a paragraph from ROM
 */
static void loadParaROM(uint8_t para, uint16_t address) {
  fprintf(stderr, "     Loading ROM paragraph %d from 0x%04x\n", para, address);
  uint32_t offset = (uint32_t)(address & 0x0FFF) << 7;
  uint8_t *pTarget = &g_paragraphs.m_paraData[para * PARA_SIZE];
  for(uint8_t index=0; index<PARA_SIZE; index++, pTarget++, offset++)
    *pTarget = g_ROM[offset];
  }

/** Load a paragraph from RAM
 */
static void loadParaRAM(uint8_t para, uint16_t address) {
  fprintf(stderr, "     Loading RAM paragraph %d from 0x%04x\n", para, address);
  uint32_t offset = (uint32_t)(address & 0x0FFF) << 7;
  uint8_t *pTarget = &g_paragraphs.m_paraData[para * PARA_SIZE];
  for(uint8_t index=0; index<PARA_SIZE; index++, pTarget++, offset++)
    *pTarget = g_RAM[offset];
  }

/** Save a paragraph to RAM
 */
static void saveParaRAM(uint8_t para, uint16_t address) {
  fprintf(stderr, "     Saving RAM paragraph %d to 0x%04x\n", para, address);
  uint32_t offset = (uint32_t)(address & 0x0FFF) << 7;
  uint8_t *pTarget = &g_paragraphs.m_paraData[para * PARA_SIZE];
  for(uint8_t index=0; index<PARA_SIZE; index++, pTarget++, offset++)
    g_RAM[offset] = *pTarget;
  // Clear the dirty flag
  g_paragraphs.m_paraInfo[para] &= ~((uint16_t)PARA_DIRTY);
  }

/** Bump the given parage to the most recently used
 */
static void bumpParagraph(uint8_t para) {
  uint8_t index;
  // Quick exit if already at top
  if((g_paragraphs.m_paraInfo[para]&PARA_SEQUENCE)==0)
    return;
  // Re-order them
  for(index=0; index<PARA_COUNT; index++) {
    if((g_paragraphs.m_paraInfo[index] & PARA_SEQUENCE)!=0x03)
      g_paragraphs.m_paraInfo[index]++;
    }
  g_paragraphs.m_paraInfo[para] &= ~(uint16_t)PARA_SEQUENCE;
  fprintf(stderr, "     Bumped paragraph %d (%d, %d, %d, %d)\n",
    para,
    g_paragraphs.m_paraInfo[0]&PARA_SEQUENCE,
    g_paragraphs.m_paraInfo[1]&PARA_SEQUENCE,
    g_paragraphs.m_paraInfo[2]&PARA_SEQUENCE,
    g_paragraphs.m_paraInfo[3]&PARA_SEQUENCE
    );
  }

/** Get the paragraph for the given CPU address
 */
static uint8_t getParagraph(uint16_t address) {
  // Calculate the paragraph address (shifted left by 3 bits)
  uint16_t para = (((uint16_t)g_ioState.m_pages[(address >> 13)] << 9) & 0xFE00) | ((address >> 4) & 0x01F8);
  fprintf(stderr, "     Paragraph: %04x\n", para >> 3);
  // Do we have it in the table?
  uint8_t index;
  for(index=0; index<PARA_COUNT; index++)
    if((g_paragraphs.m_paraInfo[index] & PARA_ADDRESS)==para) {
      fprintf(stderr, "     Found in paragraph %d\n", index);
      bumpParagraph(index);
      return index;
      }
  // Get the oldest paragraph and swap it out
  for(index=0; (index<PARA_COUNT) && ((g_paragraphs.m_paraInfo[index]&PARA_SEQUENCE)!=3) ; index++);
  if(index>=PARA_COUNT) {
    fprintf(stderr, "FATAL: Could not find paragraph sequence 3 (%d, %d, %d, %d)\n",
      g_paragraphs.m_paraInfo[0]&PARA_SEQUENCE,
      g_paragraphs.m_paraInfo[1]&PARA_SEQUENCE,
      g_paragraphs.m_paraInfo[2]&PARA_SEQUENCE,
      g_paragraphs.m_paraInfo[3]&PARA_SEQUENCE
      );
    exit(1);
    }
  bumpParagraph(index);
  // Set the new address and offset but keep the dirty flag
  g_paragraphs.m_paraInfo[index] = para | (g_paragraphs.m_paraInfo[index] & PARA_DIRTY);
  para = para >> 3;
  if(g_paragraphs.m_paraInfo[index]&PARA_ROM)
    loadParaROM(index, para);
  else {
    if(g_paragraphs.m_paraInfo[index]&PARA_DIRTY)
      saveParaRAM(index, para);
    loadParaRAM(index, para);
    }
  // All done
  fprintf(stderr, "     Found in paragraph %d\n", index);
  return index;
  }

/** Get the physical (21 bit) address for the given CPU address
*
* This function uses the page map (defined in the IO region) to convert the
* 16 bit CPU address into a 21 bit physical address.
*/
uint32_t cpuPhysicalAddress(uint16_t address) {
  uint32_t result = (uint32_t)(address & 0x1FFF);
  uint32_t page = (uint32_t)g_ioState.m_pages[(address >> 13)] << 13;
  return result | page;
  }

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
  uint32_t physical = cpuPhysicalAddress(address);
  fprintf(stderr, "MEM: Read from address 0x%04x (0x%08x) ...\n", address, physical);
  uint8_t para = getParagraph(address);
  return g_paragraphs.m_paraData[para * PARA_SIZE + (address & 0x7F)];
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
  uint32_t physical = cpuPhysicalAddress(address);
  fprintf(stderr, "MEM: Write to address 0x%04x (0x%08x) value 0x%02x ...\n", address, physical, value);
  uint8_t para = getParagraph(address);
  if(g_paragraphs.m_paraInfo[para]&0x8000)
    return;
  g_paragraphs.m_paraData[para * PARA_SIZE + (address & 0x7F)] = value;
  g_paragraphs.m_paraInfo[para] |= PARA_DIRTY;
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
  cpuResetIO();
  cpuReset();
  while (true)
    cpuStep();
  return 0;
  }
