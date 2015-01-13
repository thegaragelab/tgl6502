/*--------------------------------------------------------------------------*
* 6502 CPU Emulator
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This implementation was taken from a Stack Exchange post found here -
*  http://codegolf.stackexchange.com/questions/12844/emulate-a-mos-6502-cpu
*
* Original code is (c)2011-2013 Mike Chambers and was released as the
* Fake6502 CPU emulator core v1.1. Modifications have been made to allow it
* run on an AVR processor.
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <tgl6502.h>
#include "cpu6502.h"

//--- State information and internal variables
CPU_STATE       g_cpuState; //!< CPU state
static uint8_t  opcode;     //!< Current opcode
static uint16_t ea;         //!< Current effective address

//--- Bit definitions for the flag register
#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

//--- Start address for the stack pointer
#define BASE_STACK     0x100

//---------------------------------------------------------------------------
// Map opcodes to addressing modes and instructions
//
// NOTE: This remaps the instruction code to elimnate all 0bxxxxxx11 codes
//       reducing the table size by 25%. To get the table index from the
//       opcode use (opcode >> 2) | ((opcode & 0x03) << 6). Anything greater
//       than or equal to 192 is a NOP.
//---------------------------------------------------------------------------

typedef enum {
  MODE_NONE, MODE_ABSO, MODE_ABSX, MODE_ABSY, MODE_ACC, MODE_IMM, MODE_IMP, MODE_IND,
  MODE_INDX, MODE_INDY, MODE_REL, MODE_ZP, MODE_ZPX, MODE_ZPY
  } MODE;

static const uint8_t g_MODE[] = {
  (MODE_IMP<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,  (MODE_REL<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,  (MODE_ABSO<<4)|MODE_ZP,  (MODE_IMP<<4)|MODE_ABSO,  (MODE_REL<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,
  (MODE_IMP<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_ABSO,  (MODE_REL<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,  (MODE_IMP<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_IND,   (MODE_REL<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,
  (MODE_NONE<<4)|MODE_ZP,  (MODE_IMP<<4)|MODE_ABSO,  (MODE_REL<<4)|MODE_ZPX,  (MODE_IMP<<4)|MODE_NONE,  (MODE_IMM<<4)|MODE_ZP,   (MODE_IMP<<4)|MODE_ABSO,  (MODE_REL<<4)|MODE_ZPX,  (MODE_IMP<<4)|MODE_ABSX,
  (MODE_IMM<<4)|MODE_ZP,   (MODE_IMP<<4)|MODE_ABSO,  (MODE_REL<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,  (MODE_IMM<<4)|MODE_ZP,   (MODE_IMP<<4)|MODE_ABSO,  (MODE_REL<<4)|MODE_NONE, (MODE_IMP<<4)|MODE_NONE,
  (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX, (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX,
  (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX, (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX,
  (MODE_INDX<<4)|MODE_ZP,  (MODE_NONE<<4)|MODE_ABSO, (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX, (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX,
  (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX, (MODE_INDX<<4)|MODE_ZP,  (MODE_IMM<<4)|MODE_ABSO,  (MODE_INDY<<4)|MODE_ZPX, (MODE_ABSY<<4)|MODE_ABSX,
  (MODE_NONE<<4)|MODE_ZP,  (MODE_ACC<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPX, (MODE_NONE<<4)|MODE_ABSX, (MODE_NONE<<4)|MODE_ZP,  (MODE_ACC<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPX, (MODE_NONE<<4)|MODE_ABSX,
  (MODE_NONE<<4)|MODE_ZP,  (MODE_ACC<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPX, (MODE_NONE<<4)|MODE_ABSX, (MODE_NONE<<4)|MODE_ZP,  (MODE_ACC<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPX, (MODE_NONE<<4)|MODE_ABSX,
  (MODE_NONE<<4)|MODE_ZP,  (MODE_IMP<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPY, (MODE_IMP<<4)|MODE_NONE,  (MODE_IMM<<4)|MODE_ZP,   (MODE_IMP<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPY, (MODE_IMP<<4)|MODE_ABSY,
  (MODE_NONE<<4)|MODE_ZP,  (MODE_IMP<<4)|MODE_ABSO,  (MODE_NONE<<4)|MODE_ZPX, (MODE_NONE<<4)|MODE_ABSX, (MODE_NONE<<4)|MODE_ZP,  (MODE_NONE<<4)|MODE_ABSO, (MODE_NONE<<4)|MODE_ZPX, (MODE_NONE<<4)|MODE_ABSX
  };

/* Opcode execution */
typedef enum {
  OPCODE_ADC, OPCODE_AND, OPCODE_ASL, OPCODE_BCC, OPCODE_BCS, OPCODE_BEQ, OPCODE_BIT,
  OPCODE_BMI, OPCODE_BNE, OPCODE_BPL, OPCODE_BRK, OPCODE_BVC, OPCODE_BVS, OPCODE_CLC,
  OPCODE_CLD, OPCODE_CLI, OPCODE_CLV, OPCODE_CMP, OPCODE_CPX, OPCODE_CPY, OPCODE_DCP,
  OPCODE_DEC, OPCODE_DEX, OPCODE_DEY, OPCODE_EOR, OPCODE_INC, OPCODE_INX, OPCODE_INY,
  OPCODE_ISB, OPCODE_JMP, OPCODE_JSR, OPCODE_LAX, OPCODE_LDA, OPCODE_LDX, OPCODE_LDY,
  OPCODE_LSR, OPCODE_NOP, OPCODE_ORA, OPCODE_PHA, OPCODE_PHP, OPCODE_PLA, OPCODE_PLP,
  OPCODE_RLA, OPCODE_ROL, OPCODE_ROR, OPCODE_RRA, OPCODE_RTI, OPCODE_RTS, OPCODE_SAX,
  OPCODE_SBC, OPCODE_SEC, OPCODE_SED, OPCODE_SEI, OPCODE_SLO, OPCODE_SRE, OPCODE_STA,
  OPCODE_STX, OPCODE_STY, OPCODE_TAX, OPCODE_TAY, OPCODE_TSX, OPCODE_TXA, OPCODE_TXS,
  OPCODE_TYA
  } OPCODE;

/** Define the instructions to execute for each opcode.
 */
static const uint8_t g_OPCODE[] = {
  OPCODE_BRK, OPCODE_NOP, OPCODE_PHP, OPCODE_NOP, OPCODE_BPL, OPCODE_NOP, OPCODE_CLC, OPCODE_NOP,
  OPCODE_JSR, OPCODE_BIT, OPCODE_PLP, OPCODE_BIT, OPCODE_BMI, OPCODE_NOP, OPCODE_SEC, OPCODE_NOP,
  OPCODE_RTI, OPCODE_NOP, OPCODE_PHA, OPCODE_JMP, OPCODE_BVC, OPCODE_NOP, OPCODE_CLI, OPCODE_NOP,
  OPCODE_RTS, OPCODE_NOP, OPCODE_PLA, OPCODE_JMP, OPCODE_BVS, OPCODE_NOP, OPCODE_SEI, OPCODE_NOP,
  OPCODE_NOP, OPCODE_STY, OPCODE_DEY, OPCODE_STY, OPCODE_BCC, OPCODE_STY, OPCODE_TYA, OPCODE_NOP,
  OPCODE_LDY, OPCODE_LDY, OPCODE_TAY, OPCODE_LDY, OPCODE_BCS, OPCODE_LDY, OPCODE_CLV, OPCODE_LDY,
  OPCODE_CPY, OPCODE_CPY, OPCODE_INY, OPCODE_CPY, OPCODE_BNE, OPCODE_NOP, OPCODE_CLD, OPCODE_NOP,
  OPCODE_CPX, OPCODE_CPX, OPCODE_INX, OPCODE_CPX, OPCODE_BEQ, OPCODE_NOP, OPCODE_SED, OPCODE_NOP,
  OPCODE_ORA, OPCODE_ORA, OPCODE_ORA, OPCODE_ORA, OPCODE_ORA, OPCODE_ORA, OPCODE_ORA, OPCODE_ORA,
  OPCODE_AND, OPCODE_AND, OPCODE_AND, OPCODE_AND, OPCODE_AND, OPCODE_AND, OPCODE_AND, OPCODE_AND,
  OPCODE_EOR, OPCODE_EOR, OPCODE_EOR, OPCODE_EOR, OPCODE_EOR, OPCODE_EOR, OPCODE_EOR, OPCODE_EOR,
  OPCODE_ADC, OPCODE_ADC, OPCODE_ADC, OPCODE_ADC, OPCODE_ADC, OPCODE_ADC, OPCODE_ADC, OPCODE_ADC,
  OPCODE_STA, OPCODE_STA, OPCODE_NOP, OPCODE_STA, OPCODE_STA, OPCODE_STA, OPCODE_STA, OPCODE_STA,
  OPCODE_LDA, OPCODE_LDA, OPCODE_LDA, OPCODE_LDA, OPCODE_LDA, OPCODE_LDA, OPCODE_LDA, OPCODE_LDA,
  OPCODE_CMP, OPCODE_CMP, OPCODE_CMP, OPCODE_CMP, OPCODE_CMP, OPCODE_CMP, OPCODE_CMP, OPCODE_CMP,
  OPCODE_SBC, OPCODE_SBC, OPCODE_SBC, OPCODE_SBC, OPCODE_SBC, OPCODE_SBC, OPCODE_SBC, OPCODE_SBC,
  OPCODE_NOP, OPCODE_ASL, OPCODE_ASL, OPCODE_ASL, OPCODE_NOP, OPCODE_ASL, OPCODE_NOP, OPCODE_ASL,
  OPCODE_NOP, OPCODE_ROL, OPCODE_ROL, OPCODE_ROL, OPCODE_NOP, OPCODE_ROL, OPCODE_NOP, OPCODE_ROL,
  OPCODE_NOP, OPCODE_LSR, OPCODE_LSR, OPCODE_LSR, OPCODE_NOP, OPCODE_LSR, OPCODE_NOP, OPCODE_LSR,
  OPCODE_NOP, OPCODE_ROR, OPCODE_ROR, OPCODE_ROR, OPCODE_NOP, OPCODE_ROR, OPCODE_NOP, OPCODE_ROR,
  OPCODE_NOP, OPCODE_STX, OPCODE_TXA, OPCODE_STX, OPCODE_NOP, OPCODE_STX, OPCODE_TXS, OPCODE_NOP,
  OPCODE_LDX, OPCODE_LDX, OPCODE_TAX, OPCODE_LDX, OPCODE_NOP, OPCODE_LDX, OPCODE_TSX, OPCODE_LDX,
  OPCODE_NOP, OPCODE_DEC, OPCODE_DEX, OPCODE_DEC, OPCODE_NOP, OPCODE_DEC, OPCODE_NOP, OPCODE_DEC,
  OPCODE_NOP, OPCODE_INC, OPCODE_NOP, OPCODE_INC, OPCODE_NOP, OPCODE_INC, OPCODE_NOP, OPCODE_INC
  };

//---------------------------------------------------------------------------
// Helper functions and macros
//---------------------------------------------------------------------------

//--- Save the value to the accumulator
#define saveaccum(n) g_cpuState.m_a = (uint8_t)((n) & 0x00FF)

//--- Flag modifier macros
#define setcarry() g_cpuState.m_status |= FLAG_CARRY
#define clearcarry() g_cpuState.m_status &= (~FLAG_CARRY)
#define setzero() g_cpuState.m_status |= FLAG_ZERO
#define clearzero() g_cpuState.m_status &= (~FLAG_ZERO)
#define setinterrupt() g_cpuState.m_status |= FLAG_INTERRUPT
#define clearinterrupt() g_cpuState.m_status &= (~FLAG_INTERRUPT)
#define setdecimal() g_cpuState.m_status |= FLAG_DECIMAL
#define cleardecimal() g_cpuState.m_status &= (~FLAG_DECIMAL)
#define setoverflow() g_cpuState.m_status |= FLAG_OVERFLOW
#define clearoverflow() g_cpuState.m_status &= (~FLAG_OVERFLOW)
#define setsign() g_cpuState.m_status |= FLAG_SIGN
#define clearsign() g_cpuState.m_status &= (~FLAG_SIGN)

/** Read a single word from the CPU address space.
*
* @param address the 16 bit address to read a value from.
*
* @return the word read from the address.
*/
static uint16_t cpuReadWord(uint16_t address) {
  (uint16_t)cpuReadByte(address) | ((uint16_t)cpuReadByte(address + 1) << 8);
  }

/** Set the zero flag based on the given value
 *
 * @param n the value to check
 */
static void zerocalc(uint16_t n) {
  if ((n) & 0x00FF)
    clearzero();
  else
    setzero();
  }

/** Set the sign flag based on the given value
 *
 * @param n the value to check
 */
static void signcalc(uint16_t n) {
  if ((n) & 0x0080)
    setsign();
  else
    clearsign();
  }

/** Set the carry flag based on the given value
 *
 * @param n the value to check
 */
static void carrycalc(uint16_t n) {
  if ((n) & 0xFF00)
    setcarry();
  else
    clearcarry();
  }

/** Set the overflow flag based on the given value
 *
 * @param n result
 * @param m accumulator
 * @param o = memory
 */
static void overflowcalc(uint16_t n, uint16_t m, uint16_t o) {
  if ((n ^ m) & (n ^ o) & 0x0080)
    setoverflow();
  else
    clearoverflow();
  }

/** Push a 16 bit value on to the stack
 *
 * @param pushval the value to push
 */
static void push16(uint16_t pushval) {
  cpuWriteByte(BASE_STACK + g_cpuState.m_sp, (pushval >> 8) & 0xFF);
  cpuWriteByte(BASE_STACK + ((g_cpuState.m_sp - 1) & 0xFF), pushval & 0xFF);
  g_cpuState.m_sp -= 2;
  }

/** Push an 8 bit value on to the stack
 *
 * @param pushval the value to push
 */
static void push8(uint8_t pushval) {
  cpuWriteByte(BASE_STACK + g_cpuState.m_sp--, pushval);
  }

/** Pop a 16 bit value from the stack
 *
 * @return the value popped
 */
static uint16_t pull16() {
  uint16_t temp16;
  temp16 = cpuReadWord(BASE_STACK + ((g_cpuState.m_sp + 1) & 0xFF));
  g_cpuState.m_sp += 2;
  return(temp16);
  }

/** Pop an 8 bit value from the stack
 *
 * @return the value popped
 */
static uint8_t pull8() {
  return (cpuReadByte(BASE_STACK + ++g_cpuState.m_sp));
  }

/** Get the value at the current effective address
 *
 * @return the value at the address according to addressing mode
 */
static uint16_t getvalue() {
  if ((opcode==0x0a)||(opcode==0x2a)||(opcode==0x4a)||(opcode==0x6a))
    return((uint16_t)g_cpuState.m_a);
  else
    return((uint16_t)cpuReadByte(ea));
  }

/** Store a value at the current effective address
 *
 * @param value the value to store
 */
static void putvalue(uint16_t saveval) {
  if ((opcode==0x0a)||(opcode==0x2a)||(opcode==0x4a)||(opcode==0x6a))
    g_cpuState.m_a = (uint8_t)(saveval & 0x00FF);
  else
    cpuWriteByte(ea, (saveval & 0x00FF));
  }

//---------------------------------------------------------------------------
// Core emulator implementation
//---------------------------------------------------------------------------

/** Execute a single 6502 instruction
 *
 * This single massive function makes up the bulk of the emulator core. The
 * original source was much nicer but as part of the space saving optimisations
 * I had to move everything into here.
 */
void cpuStep() {
  uint8_t id;
  uint16_t eahelp, eahelp2, reladdr, value, result;
  opcode = cpuReadByte(g_cpuState.m_pc++);
  uint8_t mapped = (opcode >> 2) | ((opcode & 0x03) << 6);
  if(mapped<192) {
    id = g_MODE[mapped >> 1];
    id = (mapped&0x01)?(id&0x0F):(id>>4);
    }
  else // NOP
    return;
  // Apply the addressing mode
  switch(id) {
    case MODE_ABSO:
      ea = cpuReadWord(g_cpuState.m_pc);
      g_cpuState.m_pc += 2;
      break;
    case MODE_ABSX:
      ea = cpuReadWord(g_cpuState.m_pc);
      ea += (uint16_t)g_cpuState.m_x;
      g_cpuState.m_pc += 2;
      break;
    case MODE_ABSY:
      ea = cpuReadWord(g_cpuState.m_pc);
      ea += (uint16_t)g_cpuState.m_y;
      g_cpuState.m_pc += 2;
      break;
    case MODE_IMM:
      ea = g_cpuState.m_pc++;
      break;
    case MODE_IND:
      eahelp = cpuReadWord(g_cpuState.m_pc);
      eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
      ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
      g_cpuState.m_pc += 2;
      break;
    case MODE_INDX:
      eahelp = (uint16_t)(((uint16_t)cpuReadByte(g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF); //zero-page wraparound for table pointer
      ea = cpuReadWord(eahelp);
      break;
    case MODE_INDY:
      eahelp = (uint16_t)cpuReadByte(g_cpuState.m_pc++);
      eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
      ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
      ea += (uint16_t)g_cpuState.m_y;
      break;
    case MODE_REL:
      reladdr = (uint16_t)cpuReadByte(g_cpuState.m_pc++);
      if (reladdr & 0x80)
        reladdr |= 0xFF00;
      break;
    case MODE_ZP:
      ea = (uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++);
      break;
    case MODE_ZPX:
      ea = ((uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF; //zero-page wraparound
      break;
    case MODE_ZPY:
      ea = ((uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_y) & 0xFF; //zero-page wraparound
      break;
    }
  // Execute the opcode
  id = g_OPCODE[mapped];
  switch(id) {
    case OPCODE_ADC:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a + value + (uint16_t)(g_cpuState.m_status & FLAG_CARRY);
      carrycalc(result);
      zerocalc(result);
      overflowcalc(result, g_cpuState.m_a, value);
      signcalc(result);
      if (g_cpuState.m_status & FLAG_DECIMAL) {
        clearcarry();

        if ((g_cpuState.m_a & 0x0F) > 0x09) {
          g_cpuState.m_a += 0x06;
          }
        if ((g_cpuState.m_a & 0xF0) > 0x90) {
          g_cpuState.m_a += 0x60;
          setcarry();
          }
        }
      saveaccum(result);
      break;
    case OPCODE_AND:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a & value;
      zerocalc(result);
      signcalc(result);
      saveaccum(result);
      break;
    case OPCODE_ASL:
      value = getvalue();
      result = value << 1;
      carrycalc(result);
      zerocalc(result);
      signcalc(result);
      putvalue(result);
      break;
    case OPCODE_BCC:
      if ((g_cpuState.m_status & FLAG_CARRY) == 0)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BCS:
      if ((g_cpuState.m_status & FLAG_CARRY) == FLAG_CARRY)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BEQ:
      if ((g_cpuState.m_status & FLAG_ZERO) == FLAG_ZERO)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BIT:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a & value;
      zerocalc(result);
      g_cpuState.m_status = (g_cpuState.m_status & 0x3F) | (uint8_t)(value & 0xC0);
      break;
    case OPCODE_BMI:
      if ((g_cpuState.m_status & FLAG_SIGN) == FLAG_SIGN)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BNE:
      if ((g_cpuState.m_status & FLAG_ZERO) == 0)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BPL:
      if ((g_cpuState.m_status & FLAG_SIGN) == 0)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BRK:
      g_cpuState.m_pc++;
      push16(g_cpuState.m_pc); //push next instruction address onto stack
      push8(g_cpuState.m_status | FLAG_BREAK); //push CPU status to stack
      setinterrupt(); //set interrupt flag
      g_cpuState.m_pc = cpuReadWord(0xFFFE);
      break;
    case OPCODE_BVC:
      if ((g_cpuState.m_status & FLAG_OVERFLOW) == 0)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_BVS:
      if ((g_cpuState.m_status & FLAG_OVERFLOW) == FLAG_OVERFLOW)
        g_cpuState.m_pc += reladdr;
      break;
    case OPCODE_CLC:
      clearcarry();
      break;
    case OPCODE_CLD:
      cleardecimal();
      break;
    case OPCODE_CLI:
      clearinterrupt();
      break;
    case OPCODE_CLV:
      clearoverflow();
      break;
    case OPCODE_CMP:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a - value;
      if (g_cpuState.m_a >= (uint8_t)(value & 0x00FF))
        setcarry();
      else
        clearcarry();
      if (g_cpuState.m_a == (uint8_t)(value & 0x00FF))
        setzero();
      else
        clearzero();
      signcalc(result);
      break;
    case OPCODE_CPX:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_x - value;
      if (g_cpuState.m_x >= (uint8_t)(value & 0x00FF))
        setcarry();
      else
        clearcarry();
      if (g_cpuState.m_x == (uint8_t)(value & 0x00FF))
        setzero();
      else
        clearzero();
      signcalc(result);
      break;
    case OPCODE_CPY:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_y - value;
      if (g_cpuState.m_y >= (uint8_t)(value & 0x00FF))
        setcarry();
      else
        clearcarry();
      if (g_cpuState.m_y == (uint8_t)(value & 0x00FF))
        setzero();
      else
        clearzero();
      signcalc(result);
      break;
    case OPCODE_DEC:
      value = getvalue();
      result = value - 1;
      zerocalc(result);
      signcalc(result);
      putvalue(result);
      break;
    case OPCODE_DEX:
      g_cpuState.m_x--;
      zerocalc(g_cpuState.m_x);
      signcalc(g_cpuState.m_x);
      break;
    case OPCODE_DEY:
      g_cpuState.m_y--;
      zerocalc(g_cpuState.m_y);
      signcalc(g_cpuState.m_y);
      break;
    case OPCODE_EOR:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a ^ value;
      zerocalc(result);
      signcalc(result);
      saveaccum(result);
      break;
    case OPCODE_INC:
      value = getvalue();
      result = value + 1;
      zerocalc(result);
      signcalc(result);
      putvalue(result);
      break;
    case OPCODE_INX:
      g_cpuState.m_x++;
      zerocalc(g_cpuState.m_x);
      signcalc(g_cpuState.m_x);
      break;
    case OPCODE_INY:
      g_cpuState.m_y++;
      zerocalc(g_cpuState.m_y);
      signcalc(g_cpuState.m_y);
      break;
    case OPCODE_JMP:
      g_cpuState.m_pc = ea;
      break;
    case OPCODE_JSR:
      push16(g_cpuState.m_pc - 1);
      g_cpuState.m_pc = ea;
      break;
    case OPCODE_LDA:
      value = getvalue();
      g_cpuState.m_a = (uint8_t)(value & 0x00FF);
      zerocalc(g_cpuState.m_a);
      signcalc(g_cpuState.m_a);
      break;
    case OPCODE_LDX:
      value = getvalue();
      g_cpuState.m_x = (uint8_t)(value & 0x00FF);
      zerocalc(g_cpuState.m_x);
      signcalc(g_cpuState.m_x);
      break;
    case OPCODE_LDY:
      value = getvalue();
      g_cpuState.m_y = (uint8_t)(value & 0x00FF);
      zerocalc(g_cpuState.m_y);
      signcalc(g_cpuState.m_y);
      break;
    case OPCODE_LSR:
      value = getvalue();
      result = value >> 1;
      if (value & 1)
        setcarry();
      else
        clearcarry();
      zerocalc(result);
      signcalc(result);
      putvalue(result);
      break;
    case OPCODE_ORA:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a | value;
      zerocalc(result);
      signcalc(result);
      saveaccum(result);
      break;
    case OPCODE_PHA:
      push8(g_cpuState.m_a);
      break;
    case OPCODE_PHP:
      push8(g_cpuState.m_status | FLAG_BREAK);
      break;
    case OPCODE_PLA:
      g_cpuState.m_a = pull8();
      zerocalc(g_cpuState.m_a);
      signcalc(g_cpuState.m_a);
      break;
    case OPCODE_PLP:
      g_cpuState.m_status = pull8() | FLAG_CONSTANT;
      break;
    case OPCODE_ROL:
      value = getvalue();
      result = (value << 1) | (g_cpuState.m_status & FLAG_CARRY);
      carrycalc(result);
      zerocalc(result);
      signcalc(result);
      putvalue(result);
      break;
    case OPCODE_ROR:
      value = getvalue();
      result = (value >> 1) | ((g_cpuState.m_status & FLAG_CARRY) << 7);
      if (value & 1)
        setcarry();
      else
        clearcarry();
      zerocalc(result);
      signcalc(result);
      putvalue(result);
      break;
    case OPCODE_RTI:
      g_cpuState.m_status = pull8();
      value = pull16();
      g_cpuState.m_pc = value;
      break;
    case OPCODE_RTS:
      value = pull16();
      g_cpuState.m_pc = value + 1;
      break;
    case OPCODE_SBC:
      value = getvalue() ^ 0x00FF;
      result = (uint16_t)g_cpuState.m_a + value + (uint16_t)(g_cpuState.m_status & FLAG_CARRY);
      carrycalc(result);
      zerocalc(result);
      overflowcalc(result, g_cpuState.m_a, value);
      signcalc(result);
      if (g_cpuState.m_status & FLAG_DECIMAL) {
        clearcarry();
        g_cpuState.m_a -= 0x66;
        if ((g_cpuState.m_a & 0x0F) > 0x09) {
          g_cpuState.m_a += 0x06;
          }
        if ((g_cpuState.m_a & 0xF0) > 0x90) {
          g_cpuState.m_a += 0x60;
          setcarry();
          }
        }
      saveaccum(result);
      break;
    case OPCODE_SEC:
      setcarry();
      break;
    case OPCODE_SED:
      setdecimal();
      break;
    case OPCODE_SEI:
      setinterrupt();
      break;
    case OPCODE_STA:
      putvalue(g_cpuState.m_a);
      break;
    case OPCODE_STX:
      putvalue(g_cpuState.m_x);
      break;
    case OPCODE_STY:
      putvalue(g_cpuState.m_y);
      break;
    case OPCODE_TAX:
      g_cpuState.m_x = g_cpuState.m_a;
      zerocalc(g_cpuState.m_x);
      signcalc(g_cpuState.m_x);
      break;
    case OPCODE_TAY:
      g_cpuState.m_y = g_cpuState.m_a;
      zerocalc(g_cpuState.m_y);
      signcalc(g_cpuState.m_y);
      break;
    case OPCODE_TSX:
      g_cpuState.m_x = g_cpuState.m_sp;
      zerocalc(g_cpuState.m_x);
      signcalc(g_cpuState.m_x);
      break;
    case OPCODE_TXA:
      g_cpuState.m_a = g_cpuState.m_x;
      zerocalc(g_cpuState.m_a);
      signcalc(g_cpuState.m_a);
      break;
    case OPCODE_TXS:
      g_cpuState.m_sp = g_cpuState.m_x;
      break;
    case OPCODE_TYA:
      g_cpuState.m_a = g_cpuState.m_y;
      zerocalc(g_cpuState.m_a);
      signcalc(g_cpuState.m_a);
      break;
    }
  }

/** Trigger an interrupt
 *
 * @param interrupt the type of interrupt to generate
 */
void cpuInterrupt(INTERRUPT interrupt) {
  if(g_cpuState.m_status&FLAG_INTERRUPT)
    return; // Interrupt already pending
  // Generate the interrupt
  push16(g_cpuState.m_pc);
  push8(g_cpuState.m_status);
  g_cpuState.m_status |= FLAG_INTERRUPT;
  g_cpuState.m_pc = cpuReadWord((interrupt==INT_NMI)?0xFFFA:0xFFFE);
  }

/** Reset the CPU
 *
 * Simulates a hardware reset.
 */
void cpuReset() {
  // Reset the CPU state
  g_cpuState.m_pc = cpuReadWord(0xFFFC);
  g_cpuState.m_a = 0;
  g_cpuState.m_x = 0;
  g_cpuState.m_y = 0;
  g_cpuState.m_sp = 0xFD;
  g_cpuState.m_status |= FLAG_CONSTANT;
  }

