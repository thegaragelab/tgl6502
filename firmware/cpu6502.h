/*--------------------------------------------------------------------------*
* 6502 CPU Emulator
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* Interface to the 6502 emulator core.
*--------------------------------------------------------------------------*/
#ifndef __CPU6502_H
#define __CPU6502_H

#ifdef __cplusplus
extern "C" {
#endif

uint8_t readChar();
void writeChar(uint8_t ch);

//---------------------------------------------------------------------------
// State information
//---------------------------------------------------------------------------

/** CPU state information
 *
 * This structure keeps track of the CPU state information - all registers
 * and flags.
 */
typedef struct _CPU_STATE {
  uint16_t m_pc;     //!< Program counter
  uint8_t  m_sp;     //!< Stack pointer
  uint8_t  m_a;      //!< Accumulator
  uint8_t  m_x;      //!< X register
  uint8_t  m_y;      //!< Y register
  uint8_t  m_status; //!< Flags and status
  } CPU_STATE;

//! The global state information
extern CPU_STATE g_cpuState;

/** Represents the memory mapped IO area
 *
 * This structure maintains data for the memory mapped IO area.
 */
typedef struct _IO_STATE {
  uint8_t m_pages[8]; //!< Memory mapping
  } IO_STATE;

//! The global IO state information
extern IO_STATE g_ioState;

/** This structure contains statistical information for the memory manager
 *
 * As paragraphs are loaded and saved these counts will be updated. They can
 * be used to measure performance of the memory management unit.
 */
typedef struct _MEMORY_STATISTICS {
  uint32_t m_reads;  //!< Total number of read operations
  uint32_t m_writes; //!< Total number of write operations
  uint32_t m_hit;    //!< Number of cache hits
  uint32_t m_miss;   //!< Number of chache misses
  uint32_t m_loads;  //!< Number of loads (ROM/RAM to active paragraph table)
  uint32_t m_saves;  //!< Number of saves (active paragraph table to ROM/RAM)
  } MEMORY_STATISTICS;

extern MEMORY_STATISTICS g_memoryStats;

//---------------------------------------------------------------------------
// Memory and IO access
//
// The emulator host provides access to up to 2Mb of memory (1Mb RAM and 1Mb
// ROM) in 8K pages. There are 8 such pages visible to the CPU at any time
// and controlling the pages is done through a memory mapped page map.
//---------------------------------------------------------------------------

/** Initialise the memory and IO subsystem
 *
 * This function should be called every time the 'cpuReset()' function is
 * called. In ensures the memory mapped IO region is simulating the 'power on'
 * state.
 */
void cpuResetIO();

/** Read a single byte from the CPU address space.
*
* @param address the 16 bit address to read a value from.
*
* @return the byte read from the address.
*/
uint8_t cpuReadByte(uint16_t address);

/** Write a single byte to the CPU address space.
*
* @param address the 16 bit address to write a value to
*/
void cpuWriteByte(uint16_t address, uint8_t value);

//---------------------------------------------------------------------------
// Functions implemented by the emulator.
//---------------------------------------------------------------------------

/** Reset the CPU
 *
 * Simulates a hardware reset.
 */
void cpuReset();

/** Execute a single 6502 instruction
 */
void cpuStep();

#ifdef __cplusplus
}
#endif

#endif /* __CPU6502_H */


