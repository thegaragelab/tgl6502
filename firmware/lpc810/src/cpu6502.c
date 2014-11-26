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
static void zerocalc(uint8_t n) {
  if ((n) & 0x00FF)
    clearzero();
  else
    setzero();
  }

static void signcalc(uint8_t n) {
  if ((n) & 0x0080)
    setsign();
  else
    clearsign();
  }

static void carrycalc(uint8_t n) {
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
uint16_t oldpc, ea, reladdr, value, result;
uint8_t opcode, oldstatus;

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
static inline void imm() { //immediate
  ea = g_cpuState.m_pc++;
  }

static inline void zp() { //zero-page
  ea = (uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++);
  }

static inline void zpx() { //zero-page,X
  ea = ((uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF; //zero-page wraparound
  }

static inline void zpy() { //zero-page,Y
  ea = ((uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_y) & 0xFF; //zero-page wraparound
  }

static inline void rel() { //relative for branch ops (8-bit immediate value, sign-extended)
  reladdr = (uint16_t)cpuReadByte(g_cpuState.m_pc++);
  if (reladdr & 0x80)
    reladdr |= 0xFF00;
  }

static inline void abso() { //absolute
  ea = (uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8);
  g_cpuState.m_pc += 2;
  }

static inline void absx() { //absolute,X
  uint16_t startpage;
  ea = ((uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8));
  startpage = ea & 0xFF00;
  ea += (uint16_t)g_cpuState.m_x;
  g_cpuState.m_pc += 2;
  }

static inline void absy() { //absolute,Y
  uint16_t startpage;
  ea = ((uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8));
  startpage = ea & 0xFF00;
  ea += (uint16_t)g_cpuState.m_y;
  g_cpuState.m_pc += 2;
  }

static inline void ind() { //indirect
  uint16_t eahelp, eahelp2;
  eahelp = (uint16_t)cpuReadByte(g_cpuState.m_pc) | (uint16_t)((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8);
  eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
  ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
  g_cpuState.m_pc += 2;
  }

static inline void indx() { // (indirect,X)
  uint16_t eahelp;
  eahelp = (uint16_t)(((uint16_t)cpuReadByte(g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF); //zero-page wraparound for table pointer
  ea = (uint16_t)cpuReadByte(eahelp & 0x00FF) | ((uint16_t)cpuReadByte((eahelp+1) & 0x00FF) << 8);
  }

static inline void indy() { // (indirect),Y
  uint16_t eahelp, eahelp2, startpage;
  eahelp = (uint16_t)cpuReadByte(g_cpuState.m_pc++);
  eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
  ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
  startpage = ea & 0xFF00;
  ea += (uint16_t)g_cpuState.m_y;
  }

static uint16_t getvalue() {
  if ((opcode==0x0a)||(opcode==0x2a)||(opcode==0x4a)||(opcode==0x6a))
    return((uint16_t)g_cpuState.m_a);
  else
    return((uint16_t)cpuReadByte(ea));
  }

static inline void putvalue(uint16_t saveval) {
  if ((opcode==0x0a)||(opcode==0x2a)||(opcode==0x4a)||(opcode==0x6a))
    g_cpuState.m_a = (uint8_t)(saveval & 0x00FF);
  else
    cpuWriteByte(ea, (saveval & 0x00FF));
  }

//instruction handler functions
static inline void adc() {
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
  }

static inline void and() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a & value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
  }

static inline void asl() {
  value = getvalue();
  result = value << 1;

  carrycalc(result);
  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static inline void bcc() {
  if ((g_cpuState.m_status & FLAG_CARRY) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void bcs() {
  if ((g_cpuState.m_status & FLAG_CARRY) == FLAG_CARRY) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void beq() {
  if ((g_cpuState.m_status & FLAG_ZERO) == FLAG_ZERO) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void bit() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a & value;

  zerocalc(result);
  g_cpuState.m_status = (g_cpuState.m_status & 0x3F) | (uint8_t)(value & 0xC0);
  }

static inline void bmi() {
  if ((g_cpuState.m_status & FLAG_SIGN) == FLAG_SIGN) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void bne() {
  if ((g_cpuState.m_status & FLAG_ZERO) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void bpl() {
  if ((g_cpuState.m_status & FLAG_SIGN) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void brk() {
  g_cpuState.m_pc++;
  push16(g_cpuState.m_pc); //push next instruction address onto stack
  push8(g_cpuState.m_status | FLAG_BREAK); //push CPU status to stack
  setinterrupt(); //set interrupt flag
  g_cpuState.m_pc = (uint16_t)cpuReadByte(0xFFFE) | ((uint16_t)cpuReadByte(0xFFFF) << 8);
  }

static inline void bvc() {
  if ((g_cpuState.m_status & FLAG_OVERFLOW) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void bvs() {
  if ((g_cpuState.m_status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static inline void clc() {
  clearcarry();
  }

static inline void cld() {
  cleardecimal();
  }

static inline void cli() {
  clearinterrupt();
  }

static inline void clv() {
  clearoverflow();
  }

static inline void cmp() {
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
  }

static inline void cpx() {
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
  }

static inline void cpy() {
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
  }

static inline void dec() {
  value = getvalue();
  result = value - 1;

  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static inline void dex() {
  g_cpuState.m_x--;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static inline void dey() {
  g_cpuState.m_y--;

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static inline void eor() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a ^ value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
  }

static inline void inc() {
  value = getvalue();
  result = value + 1;

  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static inline void inx() {
  g_cpuState.m_x++;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static inline void iny() {
  g_cpuState.m_y++;

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static inline void jmp() {
  g_cpuState.m_pc = ea;
  }

static inline void jsr() {
  push16(g_cpuState.m_pc - 1);
  g_cpuState.m_pc = ea;
  }

static inline void lda() {
  value = getvalue();
  g_cpuState.m_a = (uint8_t)(value & 0x00FF);

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

static inline void ldx() {
  value = getvalue();
  g_cpuState.m_x = (uint8_t)(value & 0x00FF);

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static inline void ldy() {
  value = getvalue();
  g_cpuState.m_y = (uint8_t)(value & 0x00FF);

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static inline void lsr() {
  value = getvalue();
  result = value >> 1;

  if (value & 1)
    setcarry();
  else
    clearcarry();
  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static inline void ora() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a | value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
  }

static inline void pha() {
  push8(g_cpuState.m_a);
  }

static inline void php() {
  push8(g_cpuState.m_status | FLAG_BREAK);
  }

static inline void pla() {
  g_cpuState.m_a = pull8();

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

static inline void plp() {
  g_cpuState.m_status = pull8() | FLAG_CONSTANT;
  }

static inline void rol() {
  value = getvalue();
  result = (value << 1) | (g_cpuState.m_status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static inline void ror() {
  value = getvalue();
  result = (value >> 1) | ((g_cpuState.m_status & FLAG_CARRY) << 7);

  if (value & 1)
    setcarry();
  else
    clearcarry();
  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static inline void rti() {
  g_cpuState.m_status = pull8();
  value = pull16();
  g_cpuState.m_pc = value;
  }

static inline void rts() {
  value = pull16();
  g_cpuState.m_pc = value + 1;
  }

static inline void sbc() {
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
  }

static inline void sec() {
  setcarry();
  }

static inline void sed() {
  setdecimal();
  }

static inline void sei() {
  setinterrupt();
  }

static inline void sta() {
  putvalue(g_cpuState.m_a);
  }

static inline void stx() {
  putvalue(g_cpuState.m_x);
  }

static inline void sty() {
  putvalue(g_cpuState.m_y);
  }

static inline void tax() {
  g_cpuState.m_x = g_cpuState.m_a;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static inline void tay() {
  g_cpuState.m_y = g_cpuState.m_a;

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static inline void tsx() {
  g_cpuState.m_x = g_cpuState.m_sp;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static inline void txa() {
  g_cpuState.m_a = g_cpuState.m_x;

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

static inline void txs() {
  g_cpuState.m_sp = g_cpuState.m_x;
  }

static inline void tya() {
  g_cpuState.m_a = g_cpuState.m_y;

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

//undocumented instructions
static inline void lax() {
  lda();
  ldx();
  }

static inline void sax() {
  sta();
  stx();
  putvalue(g_cpuState.m_a & g_cpuState.m_x);
  }

static inline void dcp() {
  dec();
  cmp();
  }

static inline void isb() {
  inc();
  sbc();
  }

static inline void slo() {
  asl();
  ora();
  }

static inline void rla() {
  rol();
  and();
  }

static inline void sre() {
  lsr();
  eor();
  }

static inline void rra() {
  ror();
  adc();
  }

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

/** Apply the addressing mode
 *
 * This is very ugly and inefficient but requires far less code space than a
 * table of function pointers.
 */
static void applyMode(uint8_t opcode) {
  uint8_t id = g_MODE[opcode];
  switch(id) {
    case MODE_ABSO: abso(); break;
    case MODE_ABSX: absx(); break;
    case MODE_ABSY: absy(); break;
    case MODE_IMM: imm(); break;
    case MODE_IND: ind(); break;
    case MODE_INDX: indx(); break;
    case MODE_INDY: indy(); break;
    case MODE_REL: rel(); break;
    case MODE_ZP: zp(); break;
    case MODE_ZPX: zpx(); break;
    case MODE_ZPY: zpy(); break;
    }
  }

/** Apply the opcode
 *
 * This is very ugly and inefficient but requires far less code space than a
 * table of function pointers.
 */
static void applyOpcode(uint8_t opcode) {
  uint8_t id = g_OPCODE[opcode];
  switch(id) {
    case OPCODE_ADC: adc(); break;
    case OPCODE_AND: and(); break;
    case OPCODE_ASL: asl(); break;
    case OPCODE_BCC: bcc(); break;
    case OPCODE_BCS: bcs(); break;
    case OPCODE_BEQ: beq(); break;
    case OPCODE_BIT: bit(); break;
    case OPCODE_BMI: bmi(); break;
    case OPCODE_BNE: bne(); break;
    case OPCODE_BPL: bpl(); break;
    case OPCODE_BRK: brk(); break;
    case OPCODE_BVC: bvc(); break;
    case OPCODE_BVS: bvs(); break;
    case OPCODE_CLC: clc(); break;
    case OPCODE_CLD: cld(); break;
    case OPCODE_CLI: cli(); break;
    case OPCODE_CLV: clv(); break;
    case OPCODE_CMP: cmp(); break;
    case OPCODE_CPX: cpx(); break;
    case OPCODE_CPY: cpy(); break;
    case OPCODE_DCP: dcp(); break;
    case OPCODE_DEC: dec(); break;
    case OPCODE_DEX: dex(); break;
    case OPCODE_DEY: dey(); break;
    case OPCODE_EOR: eor(); break;
    case OPCODE_INC: inc(); break;
    case OPCODE_INX: inx(); break;
    case OPCODE_INY: iny(); break;
    case OPCODE_ISB: isb(); break;
    case OPCODE_JMP: jmp(); break;
    case OPCODE_JSR: jsr(); break;
    case OPCODE_LAX: lax(); break;
    case OPCODE_LDA: lda(); break;
    case OPCODE_LDX: ldx(); break;
    case OPCODE_LDY: ldy(); break;
    case OPCODE_LSR: lsr(); break;
    case OPCODE_ORA: ora(); break;
    case OPCODE_PHA: pha(); break;
    case OPCODE_PHP: php(); break;
    case OPCODE_PLA: pla(); break;
    case OPCODE_PLP: plp(); break;
    case OPCODE_RLA: rla(); break;
    case OPCODE_ROL: rol(); break;
    case OPCODE_ROR: ror(); break;
    case OPCODE_RRA: rra(); break;
    case OPCODE_RTI: rti(); break;
    case OPCODE_RTS: rts(); break;
    case OPCODE_SAX: sax(); break;
    case OPCODE_SBC: sbc(); break;
    case OPCODE_SEC: sec(); break;
    case OPCODE_SED: sed(); break;
    case OPCODE_SEI: sei(); break;
    case OPCODE_SLO: slo(); break;
    case OPCODE_SRE: sre(); break;
    case OPCODE_STA: sta(); break;
    case OPCODE_STX: stx(); break;
    case OPCODE_STY: sty(); break;
    case OPCODE_TAX: tax(); break;
    case OPCODE_TAY: tay(); break;
    case OPCODE_TSX: tsx(); break;
    case OPCODE_TXA: txa(); break;
    case OPCODE_TXS: txs(); break;
    case OPCODE_TYA: tya(); break;
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

void cpuStep() {
  opcode = cpuReadByte(g_cpuState.m_pc++);
  applyMode(opcode);
  applyOpcode(opcode);
  }
