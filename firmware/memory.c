/*--------------------------------------------------------------------------*
* 6502 CPU Emulator - Memory management
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This file implements the memory and IO access functions for the emulator.
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "cpu6502.h"

/* These are the addresses used for console input and output by EhBASIC
 *
 * Eventually these will get replaced with a more integrated IO emulation and
 * redirect to the serial port on the host MCU. For now they are hardcoded.
 */
#define CONSOLE_OUT 0xF001
#define CONSOLE_IN  0xF004

//! Size of RAM in emulated machine
#define RAM_SIZE (128 * 1024)

//! Size of ROM in emulated machine
#define ROM_SIZE (128 * 1024)

//! RAM memory
uint8_t g_RAM[RAM_SIZE];

//! ROM memory
uint8_t g_ROM[ROM_SIZE];

/** Memory statistics
 *
 * TODO: Only for analysis, not for final version.
 */
MEMORY_STATISTICS g_memoryStats;

//! Number of active paragraphs in host memory
#define PARA_COUNT 4

//! Size of a single paragraph
#define PARA_SIZE  128

//! Mask to extract the sequence number from a paragraph ID (for LRU ordering)
#define PARA_SEQUENCE 0x03

//! Mask to extract the 'dirty' flag for a paragraph ID.
#define PARA_DIRTY    0x04

//! Mask to extract the address from a paragraph ID.
#define PARA_ADDRESS  0xFFF8

//! Mask for all but sequence number
#define PARA_MASK     0xFFFC

//! Bit to test to see if the address is in ROM space.
#define PARA_ROM      0x8000

/** Defines the paragraph table
 *
 * This contains a paragraph ID and the data for each paragraph. This is
 * permanently in memory so should be kept relatively small. With 4 x 128 byte
 * paragraphs we have a total of 520 bytes - more than half the available RAM.
 */
typedef struct _PARA_TABLE {
  uint16_t m_paraInfo[PARA_COUNT]; //!< Information about loaded paragraphs
  uint8_t  m_paraData[PARA_COUNT * PARA_SIZE]; //!< Cached paragraphs
  } PARA_TABLE;

//! The actual paragraph table
static PARA_TABLE g_paragraphs;

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
  uint32_t offset = (uint32_t)(address & 0x0FFF) << 7;
  uint8_t *pTarget = &g_paragraphs.m_paraData[para * PARA_SIZE];
  for(uint8_t index=0; index<PARA_SIZE; index++, pTarget++, offset++)
    *pTarget = g_ROM[offset];
  g_memoryStats.m_loads++;
  g_paragraphs.m_paraInfo[para] = address << 3;
  }

/** Load a paragraph from RAM
 */
static void loadParaRAM(uint8_t para, uint16_t address) {
  uint32_t offset = (uint32_t)(address & 0x0FFF) << 7;
  uint8_t *pTarget = &g_paragraphs.m_paraData[para * PARA_SIZE];
  for(uint8_t index=0; index<PARA_SIZE; index++, pTarget++, offset++)
    *pTarget = g_RAM[offset];
  g_memoryStats.m_loads++;
  g_paragraphs.m_paraInfo[para] = address << 3;
  }

/** Save a paragraph to RAM
 */
static void saveParaRAM(uint8_t para) {
  uint32_t offset = (uint32_t)(g_paragraphs.m_paraInfo[para] & PARA_ADDRESS) << 4;
  uint8_t *pTarget = &g_paragraphs.m_paraData[para * PARA_SIZE];
  for(uint8_t index=0; index<PARA_SIZE; index++, pTarget++, offset++)
    g_RAM[offset] = *pTarget;
  // Clear the dirty flag
  g_paragraphs.m_paraInfo[para] &= ~((uint16_t)PARA_DIRTY);
  g_memoryStats.m_saves++;
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
  }

/** Get the paragraph for the given CPU address
 */
static uint8_t getParagraph(uint16_t address) {
  // Calculate the paragraph address (shifted left by 3 bits)
  uint16_t para = (((uint16_t)g_ioState.m_pages[(address >> 13)] << 9) & 0xFE00) | ((address >> 4) & 0x01F8);
  // Do we have it in the table?
  uint8_t index;
  for(index=0; index<PARA_COUNT; index++)
    if((g_paragraphs.m_paraInfo[index] & PARA_ADDRESS)==para) {
      bumpParagraph(index);
      return index;
      }
  // Get the oldest paragraph and swap it out
  for(index=0; (index<PARA_COUNT) && ((g_paragraphs.m_paraInfo[index]&PARA_SEQUENCE)!=3) ; index++);
  bumpParagraph(index);
  // If the discarded paragraph is dirty, write it
  if(g_paragraphs.m_paraInfo[index]&PARA_DIRTY)
    saveParaRAM(index);
  // Load the new page
  if(para&PARA_ROM)
    loadParaROM(index, para >> 3);
  else
    loadParaRAM(index, para >> 3);
  // All done
  return index;
  }

/** Read a single byte from the CPU address space.
*
* @param address the 16 bit address to read a value from.
*
* @return the byte read from the address.
*/
uint8_t cpuReadByte(uint16_t address) {
  // Simulated keyboard for EhBASIC
  if (address == CONSOLE_IN)
    return readChar();
  // Read from memory
  g_memoryStats.m_reads++;
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
    writeChar(value);
    return;
    }
  // Write data to RAM only
  uint8_t para = getParagraph(address);
  if(g_paragraphs.m_paraInfo[para]&0x8000)
    return;
  g_memoryStats.m_writes++;
  g_paragraphs.m_paraData[para * PARA_SIZE + (address & 0x7F)] = value;
  g_paragraphs.m_paraInfo[para] |= PARA_DIRTY;
  }

