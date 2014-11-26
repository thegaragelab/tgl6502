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
#include <stdio.h>
#include <stdint.h>
#include "cpu6502.h"

//6502 defines
#define UNDOCUMENTED //when this is defined, undocumented opcodes are handled.
//otherwise, they're simply treated as NOPs.

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_CONSTANT  0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_SIGN      0x80

#define BASE_STACK     0x100

#define saveaccum(n) g_cpuState.m_a = (uint8_t)((n) & 0x00FF)


//flag modifier macros
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


//flag calculation macros
static void zerocalc(uint16_t n) {
  if ((n) & 0x00FF)
    clearzero();
  else
    setzero();
  }

static void signcalc(uint16_t n) {
  if ((n) & 0x0080)
    setsign();
  else
    clearsign();
  }

static void carrycalc(uint16_t n) {
  if ((n) & 0xFF00)
    setcarry();
  else
    clearcarry();
  }

#define overflowcalc(n, m, o) { /* n = result, m = accumulator, o = memory */ \
    if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080) setoverflow();\
        else clearoverflow();\
}

// 6502 CPU registers
CPU_STATE g_cpuState;

//helper variables
static uint8_t opcode; //!< Current opcode
static uint16_t ea;    //!< Current effective address

//a few general functions used by various other functions
static void push16(uint16_t pushval) {
  cpuWriteByte(BASE_STACK + g_cpuState.m_sp, (pushval >> 8) & 0xFF);
  cpuWriteByte(BASE_STACK + ((g_cpuState.m_sp - 1) & 0xFF), pushval & 0xFF);
  g_cpuState.m_sp -= 2;
  }

static void push8(uint8_t pushval) {
  cpuWriteByte(BASE_STACK + g_cpuState.m_sp--, pushval);
  }

static uint16_t pull16() {
  uint16_t temp16;
  temp16 = cpuReadByte(BASE_STACK + ((g_cpuState.m_sp + 1) & 0xFF)) | ((uint16_t)cpuReadByte(BASE_STACK + ((g_cpuState.m_sp + 2) & 0xFF)) << 8);
  g_cpuState.m_sp += 2;
  return(temp16);
  }

static uint8_t pull8() {
  return (cpuReadByte(BASE_STACK + ++g_cpuState.m_sp));
  }

//addressing mode functions, calculates effective addresses

static uint16_t getvalue() {
  if ((opcode==0x0a)||(opcode==0x2a)||(opcode==0x4a)||(opcode==0x6a))
    return((uint16_t)g_cpuState.m_a);
  else
    return((uint16_t)cpuReadByte(ea));
  }

static void putvalue(uint16_t saveval) {
  if ((opcode==0x0a)||(opcode==0x2a)||(opcode==0x4a)||(opcode==0x6a))
    g_cpuState.m_a = (uint8_t)(saveval & 0x00FF);
  else
    cpuWriteByte(ea, (saveval & 0x00FF));
  }

//instruction handler functions





/* Address mapping */
typedef enum {
  MODE_ABSO,
  MODE_ABSX,
  MODE_ABSY,
  MODE_ACC,
  MODE_IMM,
  MODE_IMP,
  MODE_IND,
  MODE_INDX,
  MODE_INDY,
  MODE_REL,
  MODE_ZP,
  MODE_ZPX,
  MODE_ZPY
  } MODE;

static uint8_t g_MODE[] = {
  MODE_IMP, MODE_INDX, MODE_IMP, MODE_INDX, MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP,
  MODE_IMP, MODE_IMM, MODE_ACC, MODE_IMM, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_ABSO,
  MODE_REL, MODE_INDY, MODE_IMP, MODE_INDY, MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_ZPX,
  MODE_IMP, MODE_ABSY, MODE_IMP, MODE_ABSY, MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_ABSX,
  MODE_ABSO, MODE_INDX, MODE_IMP, MODE_INDX, MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP,
  MODE_IMP, MODE_IMM, MODE_ACC, MODE_IMM, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_ABSO,
  MODE_REL, MODE_INDY, MODE_IMP, MODE_INDY, MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_ZPX,
  MODE_IMP, MODE_ABSY, MODE_IMP, MODE_ABSY, MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_ABSX,
  MODE_IMP, MODE_INDX, MODE_IMP, MODE_INDX, MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP, MODE_IMP,
  MODE_IMM, MODE_ACC, MODE_IMM, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_REL,
  MODE_INDY, MODE_IMP, MODE_INDY, MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_IMP,
  MODE_ABSY, MODE_IMP, MODE_ABSY, MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_IMP,
  MODE_INDX, MODE_IMP, MODE_INDX, MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP, MODE_IMP, MODE_IMM,
  MODE_ACC, MODE_IMM, MODE_IND, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_REL, MODE_INDY,
  MODE_IMP, MODE_INDY, MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_IMP, MODE_ABSY,
  MODE_IMP, MODE_ABSY, MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_IMM, MODE_INDX,
  MODE_IMM, MODE_INDX, MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP, MODE_IMP, MODE_IMM, MODE_IMP,
  MODE_IMM, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_REL, MODE_INDY, MODE_IMP,
  MODE_INDY, MODE_ZPX, MODE_ZPX, MODE_ZPY, MODE_ZPY, MODE_IMP, MODE_ABSY, MODE_IMP,
  MODE_ABSY, MODE_ABSX, MODE_ABSX, MODE_ABSY, MODE_ABSY, MODE_IMM, MODE_INDX, MODE_IMM,
  MODE_INDX, MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP, MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM,
  MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_REL, MODE_INDY, MODE_IMP, MODE_INDY,
  MODE_ZPX, MODE_ZPX, MODE_ZPY, MODE_ZPY, MODE_IMP, MODE_ABSY, MODE_IMP, MODE_ABSY,
  MODE_ABSX, MODE_ABSX, MODE_ABSY, MODE_ABSY, MODE_IMM, MODE_INDX, MODE_IMM, MODE_INDX,
  MODE_ZP, MODE_ZP, MODE_ZP, MODE_ZP, MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM, MODE_ABSO,
  MODE_ABSO, MODE_ABSO, MODE_ABSO, MODE_REL, MODE_INDY, MODE_IMP, MODE_INDY, MODE_ZPX,
  MODE_ZPX, MODE_ZPX, MODE_ZPX, MODE_IMP, MODE_ABSY, MODE_IMP, MODE_ABSY, MODE_ABSX,
  MODE_ABSX, MODE_ABSX, MODE_ABSX, MODE_IMM, MODE_INDX, MODE_IMM, MODE_INDX, MODE_ZP,
  MODE_ZP, MODE_ZP, MODE_ZP, MODE_IMP, MODE_IMM, MODE_IMP, MODE_IMM, MODE_ABSO, MODE_ABSO,
  MODE_ABSO, MODE_ABSO, MODE_REL, MODE_INDY, MODE_IMP, MODE_INDY, MODE_ZPX, MODE_ZPX,
  MODE_ZPX, MODE_ZPX, MODE_IMP, MODE_ABSY, MODE_IMP, MODE_ABSY, MODE_ABSX, MODE_ABSX,
  MODE_ABSX, MODE_ABSX
  };

/* Opcode execution */
typedef enum {
  OPCODE_ADC,
  OPCODE_AND,
  OPCODE_ASL,
  OPCODE_BCC,
  OPCODE_BCS,
  OPCODE_BEQ,
  OPCODE_BIT,
  OPCODE_BMI,
  OPCODE_BNE,
  OPCODE_BPL,
  OPCODE_BRK,
  OPCODE_BVC,
  OPCODE_BVS,
  OPCODE_CLC,
  OPCODE_CLD,
  OPCODE_CLI,
  OPCODE_CLV,
  OPCODE_CMP,
  OPCODE_CPX,
  OPCODE_CPY,
  OPCODE_DCP,
  OPCODE_DEC,
  OPCODE_DEX,
  OPCODE_DEY,
  OPCODE_EOR,
  OPCODE_INC,
  OPCODE_INX,
  OPCODE_INY,
  OPCODE_ISB,
  OPCODE_JMP,
  OPCODE_JSR,
  OPCODE_LAX,
  OPCODE_LDA,
  OPCODE_LDX,
  OPCODE_LDY,
  OPCODE_LSR,
  OPCODE_NOP,
  OPCODE_ORA,
  OPCODE_PHA,
  OPCODE_PHP,
  OPCODE_PLA,
  OPCODE_PLP,
  OPCODE_RLA,
  OPCODE_ROL,
  OPCODE_ROR,
  OPCODE_RRA,
  OPCODE_RTI,
  OPCODE_RTS,
  OPCODE_SAX,
  OPCODE_SBC,
  OPCODE_SEC,
  OPCODE_SED,
  OPCODE_SEI,
  OPCODE_SLO,
  OPCODE_SRE,
  OPCODE_STA,
  OPCODE_STX,
  OPCODE_STY,
  OPCODE_TAX,
  OPCODE_TAY,
  OPCODE_TSX,
  OPCODE_TXA,
  OPCODE_TXS,
  OPCODE_TYA
  } OPCODE;

static uint8_t g_OPCODE[] = {
  OPCODE_BRK, OPCODE_ORA, OPCODE_NOP, OPCODE_SLO, OPCODE_NOP, OPCODE_ORA, OPCODE_ASL, OPCODE_SLO,
  OPCODE_PHP, OPCODE_ORA, OPCODE_ASL, OPCODE_NOP, OPCODE_NOP, OPCODE_ORA, OPCODE_ASL, OPCODE_SLO,
  OPCODE_BPL, OPCODE_ORA, OPCODE_NOP, OPCODE_SLO, OPCODE_NOP, OPCODE_ORA, OPCODE_ASL, OPCODE_SLO,
  OPCODE_CLC, OPCODE_ORA, OPCODE_NOP, OPCODE_SLO, OPCODE_NOP, OPCODE_ORA, OPCODE_ASL, OPCODE_SLO,
  OPCODE_JSR, OPCODE_AND, OPCODE_NOP, OPCODE_RLA, OPCODE_BIT, OPCODE_AND, OPCODE_ROL, OPCODE_RLA,
  OPCODE_PLP, OPCODE_AND, OPCODE_ROL, OPCODE_NOP, OPCODE_BIT, OPCODE_AND, OPCODE_ROL, OPCODE_RLA,
  OPCODE_BMI, OPCODE_AND, OPCODE_NOP, OPCODE_RLA, OPCODE_NOP, OPCODE_AND, OPCODE_ROL, OPCODE_RLA,
  OPCODE_SEC, OPCODE_AND, OPCODE_NOP, OPCODE_RLA, OPCODE_NOP, OPCODE_AND, OPCODE_ROL, OPCODE_RLA,
  OPCODE_RTI, OPCODE_EOR, OPCODE_NOP, OPCODE_SRE, OPCODE_NOP, OPCODE_EOR, OPCODE_LSR, OPCODE_SRE,
  OPCODE_PHA, OPCODE_EOR, OPCODE_LSR, OPCODE_NOP, OPCODE_JMP, OPCODE_EOR, OPCODE_LSR, OPCODE_SRE,
  OPCODE_BVC, OPCODE_EOR, OPCODE_NOP, OPCODE_SRE, OPCODE_NOP, OPCODE_EOR, OPCODE_LSR, OPCODE_SRE,
  OPCODE_CLI, OPCODE_EOR, OPCODE_NOP, OPCODE_SRE, OPCODE_NOP, OPCODE_EOR, OPCODE_LSR, OPCODE_SRE,
  OPCODE_RTS, OPCODE_ADC, OPCODE_NOP, OPCODE_RRA, OPCODE_NOP, OPCODE_ADC, OPCODE_ROR, OPCODE_RRA,
  OPCODE_PLA, OPCODE_ADC, OPCODE_ROR, OPCODE_NOP, OPCODE_JMP, OPCODE_ADC, OPCODE_ROR, OPCODE_RRA,
  OPCODE_BVS, OPCODE_ADC, OPCODE_NOP, OPCODE_RRA, OPCODE_NOP, OPCODE_ADC, OPCODE_ROR, OPCODE_RRA,
  OPCODE_SEI, OPCODE_ADC, OPCODE_NOP, OPCODE_RRA, OPCODE_NOP, OPCODE_ADC, OPCODE_ROR, OPCODE_RRA,
  OPCODE_NOP, OPCODE_STA, OPCODE_NOP, OPCODE_SAX, OPCODE_STY, OPCODE_STA, OPCODE_STX, OPCODE_SAX,
  OPCODE_DEY, OPCODE_NOP, OPCODE_TXA, OPCODE_NOP, OPCODE_STY, OPCODE_STA, OPCODE_STX, OPCODE_SAX,
  OPCODE_BCC, OPCODE_STA, OPCODE_NOP, OPCODE_NOP, OPCODE_STY, OPCODE_STA, OPCODE_STX, OPCODE_SAX,
  OPCODE_TYA, OPCODE_STA, OPCODE_TXS, OPCODE_NOP, OPCODE_NOP, OPCODE_STA, OPCODE_NOP, OPCODE_NOP,
  OPCODE_LDY, OPCODE_LDA, OPCODE_LDX, OPCODE_LAX, OPCODE_LDY, OPCODE_LDA, OPCODE_LDX, OPCODE_LAX,
  OPCODE_TAY, OPCODE_LDA, OPCODE_TAX, OPCODE_NOP, OPCODE_LDY, OPCODE_LDA, OPCODE_LDX, OPCODE_LAX,
  OPCODE_BCS, OPCODE_LDA, OPCODE_NOP, OPCODE_LAX, OPCODE_LDY, OPCODE_LDA, OPCODE_LDX, OPCODE_LAX,
  OPCODE_CLV, OPCODE_LDA, OPCODE_TSX, OPCODE_LAX, OPCODE_LDY, OPCODE_LDA, OPCODE_LDX, OPCODE_LAX,
  OPCODE_CPY, OPCODE_CMP, OPCODE_NOP, OPCODE_DCP, OPCODE_CPY, OPCODE_CMP, OPCODE_DEC, OPCODE_DCP,
  OPCODE_INY, OPCODE_CMP, OPCODE_DEX, OPCODE_NOP, OPCODE_CPY, OPCODE_CMP, OPCODE_DEC, OPCODE_DCP,
  OPCODE_BNE, OPCODE_CMP, OPCODE_NOP, OPCODE_DCP, OPCODE_NOP, OPCODE_CMP, OPCODE_DEC, OPCODE_DCP,
  OPCODE_CLD, OPCODE_CMP, OPCODE_NOP, OPCODE_DCP, OPCODE_NOP, OPCODE_CMP, OPCODE_DEC, OPCODE_DCP,
  OPCODE_CPX, OPCODE_SBC, OPCODE_NOP, OPCODE_ISB, OPCODE_CPX, OPCODE_SBC, OPCODE_INC, OPCODE_ISB,
  OPCODE_INX, OPCODE_SBC, OPCODE_NOP, OPCODE_SBC, OPCODE_CPX, OPCODE_SBC, OPCODE_INC, OPCODE_ISB,
  OPCODE_BEQ, OPCODE_SBC, OPCODE_NOP, OPCODE_ISB, OPCODE_NOP, OPCODE_SBC, OPCODE_INC, OPCODE_ISB,
  OPCODE_SED, OPCODE_SBC, OPCODE_NOP, OPCODE_ISB, OPCODE_NOP, OPCODE_SBC, OPCODE_INC, OPCODE_ISB
  };


/** Execute a single 6502 instruction
 *
 * This single massive function makes up the bulk of the emulator core. The
 * original source was much nicer but as part of the space saving optimisations
 * I had to move everything into here.
 */
void cpuStep() {
  uint16_t eahelp, eahelp2, oldpc, reladdr, value, result;
  opcode = cpuReadByte(g_cpuState.m_pc++);
  uint8_t id = g_MODE[opcode];
  // Apply the addressing mode
  switch(id) {
    case MODE_ABSO:
      ea = (uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8);
      g_cpuState.m_pc += 2;
      break;
    case MODE_ABSX:
      ea = ((uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8));
      ea += (uint16_t)g_cpuState.m_x;
      g_cpuState.m_pc += 2;
      break;
    case MODE_ABSY:
      ea = ((uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8));
      ea += (uint16_t)g_cpuState.m_y;
      g_cpuState.m_pc += 2;
      break;
    case MODE_IMM:
      ea = g_cpuState.m_pc++;
      break;
    case MODE_IND:
      eahelp = (uint16_t)cpuReadByte(g_cpuState.m_pc) | (uint16_t)((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8);
      eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
      ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
      g_cpuState.m_pc += 2;
      break;
    case MODE_INDX:
      eahelp = (uint16_t)(((uint16_t)cpuReadByte(g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF); //zero-page wraparound for table pointer
      ea = (uint16_t)cpuReadByte(eahelp & 0x00FF) | ((uint16_t)cpuReadByte((eahelp+1) & 0x00FF) << 8);
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
  id = g_OPCODE[opcode];
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
      if ((g_cpuState.m_status & FLAG_CARRY) == 0) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BCS:
      if ((g_cpuState.m_status & FLAG_CARRY) == FLAG_CARRY) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BEQ:
      if ((g_cpuState.m_status & FLAG_ZERO) == FLAG_ZERO) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BIT:
      value = getvalue();
      result = (uint16_t)g_cpuState.m_a & value;
      zerocalc(result);
      g_cpuState.m_status = (g_cpuState.m_status & 0x3F) | (uint8_t)(value & 0xC0);
      break;
    case OPCODE_BMI:
      if ((g_cpuState.m_status & FLAG_SIGN) == FLAG_SIGN) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BNE:
      if ((g_cpuState.m_status & FLAG_ZERO) == 0) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BPL:
      if ((g_cpuState.m_status & FLAG_SIGN) == 0) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BRK:
      g_cpuState.m_pc++;
      push16(g_cpuState.m_pc); //push next instruction address onto stack
      push8(g_cpuState.m_status | FLAG_BREAK); //push CPU status to stack
      setinterrupt(); //set interrupt flag
      g_cpuState.m_pc = (uint16_t)cpuReadByte(0xFFFE) | ((uint16_t)cpuReadByte(0xFFFF) << 8);
      break;
    case OPCODE_BVC:
      if ((g_cpuState.m_status & FLAG_OVERFLOW) == 0) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
      break;
    case OPCODE_BVS:
      if ((g_cpuState.m_status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
        oldpc =g_cpuState.m_pc;
        g_cpuState.m_pc += reladdr;
        }
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

void nmi6502() {
  push16(g_cpuState.m_pc);
  push8(g_cpuState.m_status);
  g_cpuState.m_status |= FLAG_INTERRUPT;
  g_cpuState.m_pc = (uint16_t)cpuReadByte(0xFFFA) | ((uint16_t)cpuReadByte(0xFFFB) << 8);
  }

void irq6502() {
  push16(g_cpuState.m_pc);
  push8(g_cpuState.m_status);
  g_cpuState.m_status |= FLAG_INTERRUPT;
  g_cpuState.m_pc = (uint16_t)cpuReadByte(0xFFFE) | ((uint16_t)cpuReadByte(0xFFFF) << 8);
  }

void cpuReset() {
  g_cpuState.m_pc = (uint16_t)cpuReadByte(0xFFFC) | ((uint16_t)cpuReadByte(0xFFFD) << 8);
  g_cpuState.m_a = 0;
  g_cpuState.m_x = 0;
  g_cpuState.m_y = 0;
  g_cpuState.m_sp = 0xFD;
  g_cpuState.m_status |= FLAG_CONSTANT;
  }


