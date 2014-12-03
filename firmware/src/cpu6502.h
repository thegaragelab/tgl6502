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

//---------------------------------------------------------------------------
// State information
//---------------------------------------------------------------------------

//! Number of MMU pages (8 x 8K pages)
#define MMU_PAGE_COUNT 8

//! Size of the SPI transfer buffer
#define SPI_BUFFER_SIZE 128

#pragma pack(push, 1)

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

#pragma pack(pop)

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

/** Types of interrupt that can be generated
 */
typedef enum {
  INT_IRQ, //!< The general interrupt request pin
  INT_NMI, //!< The non-maskable interrupt pin
  } INTERRUPT;

/** Reset the CPU
 *
 * Simulates a hardware reset.
 */
void cpuReset();

/** Execute a single 6502 instruction
 */
void cpuStep();

/** Trigger an interrupt
 *
 * @param interrupt the type of interrupt to generate
 */
void cpuInterrupt(INTERRUPT interrupt);

#ifdef __cplusplus
}
#endif

#endif /* __CPU6502_H */


