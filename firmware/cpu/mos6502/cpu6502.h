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

#ifdef __cplusplus
}
#endif

#endif /* __CPU6502_H */


