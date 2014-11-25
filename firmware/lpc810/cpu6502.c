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

//#define NES_CPU      //when this is defined, the binary-coded decimal (BCD)
//status flag is not honored by ADC and SBC. the 2A03
//CPU in the Nintendo Entertainment System does not
//support BCD operation.

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
#define zerocalc(n) {\
    if ((n) & 0x00FF) clearzero();\
        else setzero();\
}

#define signcalc(n) {\
    if ((n) & 0x0080) setsign();\
        else clearsign();\
}

#define carrycalc(n) {\
    if ((n) & 0xFF00) setcarry();\
        else clearcarry();\
}

#define overflowcalc(n, m, o) { /* n = result, m = accumulator, o = memory */ \
    if (((n) ^ (uint16_t)(m)) & ((n) ^ (o)) & 0x0080) setoverflow();\
        else clearoverflow();\
}


//6502 CPU registers
CPU_STATE g_cpuState;

//uint8_t status = FLAG_CONSTANT;


//helper variables
uint16_t oldpc, ea, reladdr, value, result;
uint8_t opcode, oldstatus;

//a few general functions used by various other functions
void push16(uint16_t pushval) {
  cpuWriteByte(BASE_STACK + g_cpuState.m_sp, (pushval >> 8) & 0xFF);
  cpuWriteByte(BASE_STACK + ((g_cpuState.m_sp - 1) & 0xFF), pushval & 0xFF);
  g_cpuState.m_sp -= 2;
  }

void push8(uint8_t pushval) {
  cpuWriteByte(BASE_STACK + g_cpuState.m_sp--, pushval);
  }

uint16_t pull16() {
  uint16_t temp16;
  temp16 = cpuReadByte(BASE_STACK + ((g_cpuState.m_sp + 1) & 0xFF)) | ((uint16_t)cpuReadByte(BASE_STACK + ((g_cpuState.m_sp + 2) & 0xFF)) << 8);
  g_cpuState.m_sp += 2;
  return(temp16);
  }

uint8_t pull8() {
  return (cpuReadByte(BASE_STACK + ++g_cpuState.m_sp));
  }

static void (*addrtable[256])();
static void (*optable[256])();

//addressing mode functions, calculates effective addresses
static void imp() { //implied
  }

static void acc() { //accumulator
  }

static void imm() { //immediate
  ea = g_cpuState.m_pc++;
  }

static void zp() { //zero-page
  ea = (uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++);
  }

static void zpx() { //zero-page,X
  ea = ((uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF; //zero-page wraparound
  }

static void zpy() { //zero-page,Y
  ea = ((uint16_t)cpuReadByte((uint16_t)g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_y) & 0xFF; //zero-page wraparound
  }

static void rel() { //relative for branch ops (8-bit immediate value, sign-extended)
  reladdr = (uint16_t)cpuReadByte(g_cpuState.m_pc++);
  if (reladdr & 0x80)
    reladdr |= 0xFF00;
  }

static void abso() { //absolute
  ea = (uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8);
  g_cpuState.m_pc += 2;
  }

static void absx() { //absolute,X
  uint16_t startpage;
  ea = ((uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8));
  startpage = ea & 0xFF00;
  ea += (uint16_t)g_cpuState.m_x;
  g_cpuState.m_pc += 2;
  }

static void absy() { //absolute,Y
  uint16_t startpage;
  ea = ((uint16_t)cpuReadByte(g_cpuState.m_pc) | ((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8));
  startpage = ea & 0xFF00;
  ea += (uint16_t)g_cpuState.m_y;
  g_cpuState.m_pc += 2;
  }

static void ind() { //indirect
  uint16_t eahelp, eahelp2;
  eahelp = (uint16_t)cpuReadByte(g_cpuState.m_pc) | (uint16_t)((uint16_t)cpuReadByte(g_cpuState.m_pc+1) << 8);
  eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //replicate 6502 page-boundary wraparound bug
  ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
  g_cpuState.m_pc += 2;
  }

static void indx() { // (indirect,X)
  uint16_t eahelp;
  eahelp = (uint16_t)(((uint16_t)cpuReadByte(g_cpuState.m_pc++) + (uint16_t)g_cpuState.m_x) & 0xFF); //zero-page wraparound for table pointer
  ea = (uint16_t)cpuReadByte(eahelp & 0x00FF) | ((uint16_t)cpuReadByte((eahelp+1) & 0x00FF) << 8);
  }

static void indy() { // (indirect),Y
  uint16_t eahelp, eahelp2, startpage;
  eahelp = (uint16_t)cpuReadByte(g_cpuState.m_pc++);
  eahelp2 = (eahelp & 0xFF00) | ((eahelp + 1) & 0x00FF); //zero-page wraparound
  ea = (uint16_t)cpuReadByte(eahelp) | ((uint16_t)cpuReadByte(eahelp2) << 8);
  startpage = ea & 0xFF00;
  ea += (uint16_t)g_cpuState.m_y;
  }

static uint16_t getvalue() {
  if (addrtable[opcode] == acc)
    return((uint16_t)g_cpuState.m_a);
  else
    return((uint16_t)cpuReadByte(ea));
  }

static void putvalue(uint16_t saveval) {
  if (addrtable[opcode] == acc)
    g_cpuState.m_a = (uint8_t)(saveval & 0x00FF);
  else
    cpuWriteByte(ea, (saveval & 0x00FF));
  }


//instruction handler functions
static void adc() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a + value + (uint16_t)(g_cpuState.m_status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  overflowcalc(result, g_cpuState.m_a, value);
  signcalc(result);

#ifndef NES_CPU

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
#endif

  saveaccum(result);
  }

static void and() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a & value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
  }

static void asl() {
  value = getvalue();
  result = value << 1;

  carrycalc(result);
  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static void bcc() {
  if ((g_cpuState.m_status & FLAG_CARRY) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void bcs() {
  if ((g_cpuState.m_status & FLAG_CARRY) == FLAG_CARRY) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void beq() {
  if ((g_cpuState.m_status & FLAG_ZERO) == FLAG_ZERO) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void bit() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a & value;

  zerocalc(result);
  g_cpuState.m_status = (g_cpuState.m_status & 0x3F) | (uint8_t)(value & 0xC0);
  }

static void bmi() {
  if ((g_cpuState.m_status & FLAG_SIGN) == FLAG_SIGN) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void bne() {
  if ((g_cpuState.m_status & FLAG_ZERO) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void bpl() {
  if ((g_cpuState.m_status & FLAG_SIGN) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void brk() {
  g_cpuState.m_pc++;
  push16(g_cpuState.m_pc); //push next instruction address onto stack
  push8(g_cpuState.m_status | FLAG_BREAK); //push CPU status to stack
  setinterrupt(); //set interrupt flag
  g_cpuState.m_pc = (uint16_t)cpuReadByte(0xFFFE) | ((uint16_t)cpuReadByte(0xFFFF) << 8);
  }

static void bvc() {
  if ((g_cpuState.m_status & FLAG_OVERFLOW) == 0) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void bvs() {
  if ((g_cpuState.m_status & FLAG_OVERFLOW) == FLAG_OVERFLOW) {
    oldpc =g_cpuState.m_pc;
    g_cpuState.m_pc += reladdr;
    }
  }

static void clc() {
  clearcarry();
  }

static void cld() {
  cleardecimal();
  }

static void cli() {
  clearinterrupt();
  }

static void clv() {
  clearoverflow();
  }

static void cmp() {
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

static void cpx() {
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

static void cpy() {
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

static void dec() {
  value = getvalue();
  result = value - 1;

  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static void dex() {
  g_cpuState.m_x--;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static void dey() {
  g_cpuState.m_y--;

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static void eor() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a ^ value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
  }

static void inc() {
  value = getvalue();
  result = value + 1;

  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static void inx() {
  g_cpuState.m_x++;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static void iny() {
  g_cpuState.m_y++;

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static void jmp() {
  g_cpuState.m_pc = ea;
  }

static void jsr() {
  push16(g_cpuState.m_pc - 1);
  g_cpuState.m_pc = ea;
  }

static void lda() {
  value = getvalue();
  g_cpuState.m_a = (uint8_t)(value & 0x00FF);

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

static void ldx() {
  value = getvalue();
  g_cpuState.m_x = (uint8_t)(value & 0x00FF);

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static void ldy() {
  value = getvalue();
  g_cpuState.m_y = (uint8_t)(value & 0x00FF);

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static void lsr() {
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

static void nop() {
  // Do nothing
  }

static void ora() {
  value = getvalue();
  result = (uint16_t)g_cpuState.m_a | value;

  zerocalc(result);
  signcalc(result);

  saveaccum(result);
  }

static void pha() {
  push8(g_cpuState.m_a);
  }

static void php() {
  push8(g_cpuState.m_status | FLAG_BREAK);
  }

static void pla() {
  g_cpuState.m_a = pull8();

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

static void plp() {
  g_cpuState.m_status = pull8() | FLAG_CONSTANT;
  }

static void rol() {
  value = getvalue();
  result = (value << 1) | (g_cpuState.m_status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  signcalc(result);

  putvalue(result);
  }

static void ror() {
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

static void rti() {
  g_cpuState.m_status = pull8();
  value = pull16();
  g_cpuState.m_pc = value;
  }

static void rts() {
  value = pull16();
  g_cpuState.m_pc = value + 1;
  }

static void sbc() {
  value = getvalue() ^ 0x00FF;
  result = (uint16_t)g_cpuState.m_a + value + (uint16_t)(g_cpuState.m_status & FLAG_CARRY);

  carrycalc(result);
  zerocalc(result);
  overflowcalc(result, g_cpuState.m_a, value);
  signcalc(result);

#ifndef NES_CPU

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
#endif

  saveaccum(result);
  }

static void sec() {
  setcarry();
  }

static void sed() {
  setdecimal();
  }

static void sei() {
  setinterrupt();
  }

static void sta() {
  putvalue(g_cpuState.m_a);
  }

static void stx() {
  putvalue(g_cpuState.m_x);
  }

static void sty() {
  putvalue(g_cpuState.m_y);
  }

static void tax() {
  g_cpuState.m_x = g_cpuState.m_a;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static void tay() {
  g_cpuState.m_y = g_cpuState.m_a;

  zerocalc(g_cpuState.m_y);
  signcalc(g_cpuState.m_y);
  }

static void tsx() {
  g_cpuState.m_x = g_cpuState.m_sp;

  zerocalc(g_cpuState.m_x);
  signcalc(g_cpuState.m_x);
  }

static void txa() {
  g_cpuState.m_a = g_cpuState.m_x;

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

static void txs() {
  g_cpuState.m_sp = g_cpuState.m_x;
  }

static void tya() {
  g_cpuState.m_a = g_cpuState.m_y;

  zerocalc(g_cpuState.m_a);
  signcalc(g_cpuState.m_a);
  }

//undocumented instructions
#ifdef UNDOCUMENTED
static void lax() {
  lda();
  ldx();
  }

static void sax() {
  sta();
  stx();
  putvalue(g_cpuState.m_a & g_cpuState.m_x);
  }

static void dcp() {
  dec();
  cmp();
  }

static void isb() {
  inc();
  sbc();
  }

static void slo() {
  asl();
  ora();
  }

static void rla() {
  rol();
  and();
  }

static void sre() {
  lsr();
  eor();
  }

static void rra() {
  ror();
  adc();
  }
#else
    #define lax nop
    #define sax nop
    #define dcp nop
    #define isb nop
    #define slo nop
    #define rla nop
    #define sre nop
    #define rra nop
#endif


static void (*addrtable[256])() = {
  /*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |     */
  /* 0 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 0 */
  /* 1 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 1 */
  /* 2 */    abso, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 2 */
  /* 3 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 3 */
  /* 4 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm, abso, abso, abso, abso, /* 4 */
  /* 5 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 5 */
  /* 6 */     imp, indx,  imp, indx,   zp,   zp,   zp,   zp,  imp,  imm,  acc,  imm,  ind, abso, abso, abso, /* 6 */
  /* 7 */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* 7 */
  /* 8 */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* 8 */
  /* 9 */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* 9 */
  /* A */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* A */
  /* B */     rel, indy,  imp, indy,  zpx,  zpx,  zpy,  zpy,  imp, absy,  imp, absy, absx, absx, absy, absy, /* B */
  /* C */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* C */
  /* D */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx, /* D */
  /* E */     imm, indx,  imm, indx,   zp,   zp,   zp,   zp,  imp,  imm,  imp,  imm, abso, abso, abso, abso, /* E */
  /* F */     rel, indy,  imp, indy,  zpx,  zpx,  zpx,  zpx,  imp, absy,  imp, absy, absx, absx, absx, absx  /* F */
  };

static void (*optable[256])() = {
  /*        |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |      */
  /* 0 */      brk,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  php,  ora,  asl,  nop,  nop,  ora,  asl,  slo, /* 0 */
  /* 1 */      bpl,  ora,  nop,  slo,  nop,  ora,  asl,  slo,  clc,  ora,  nop,  slo,  nop,  ora,  asl,  slo, /* 1 */
  /* 2 */      jsr,  and,  nop,  rla,  bit,  and,  rol,  rla,  plp,  and,  rol,  nop,  bit,  and,  rol,  rla, /* 2 */
  /* 3 */      bmi,  and,  nop,  rla,  nop,  and,  rol,  rla,  sec,  and,  nop,  rla,  nop,  and,  rol,  rla, /* 3 */
  /* 4 */      rti,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  pha,  eor,  lsr,  nop,  jmp,  eor,  lsr,  sre, /* 4 */
  /* 5 */      bvc,  eor,  nop,  sre,  nop,  eor,  lsr,  sre,  cli,  eor,  nop,  sre,  nop,  eor,  lsr,  sre, /* 5 */
  /* 6 */      rts,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  pla,  adc,  ror,  nop,  jmp,  adc,  ror,  rra, /* 6 */
  /* 7 */      bvs,  adc,  nop,  rra,  nop,  adc,  ror,  rra,  sei,  adc,  nop,  rra,  nop,  adc,  ror,  rra, /* 7 */
  /* 8 */      nop,  sta,  nop,  sax,  sty,  sta,  stx,  sax,  dey,  nop,  txa,  nop,  sty,  sta,  stx,  sax, /* 8 */
  /* 9 */      bcc,  sta,  nop,  nop,  sty,  sta,  stx,  sax,  tya,  sta,  txs,  nop,  nop,  sta,  nop,  nop, /* 9 */
  /* A */      ldy,  lda,  ldx,  lax,  ldy,  lda,  ldx,  lax,  tay,  lda,  tax,  nop,  ldy,  lda,  ldx,  lax, /* A */
  /* B */      bcs,  lda,  nop,  lax,  ldy,  lda,  ldx,  lax,  clv,  lda,  tsx,  lax,  ldy,  lda,  ldx,  lax, /* B */
  /* C */      cpy,  cmp,  nop,  dcp,  cpy,  cmp,  dec,  dcp,  iny,  cmp,  dex,  nop,  cpy,  cmp,  dec,  dcp, /* C */
  /* D */      bne,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp,  cld,  cmp,  nop,  dcp,  nop,  cmp,  dec,  dcp, /* D */
  /* E */      cpx,  sbc,  nop,  isb,  cpx,  sbc,  inc,  isb,  inx,  sbc,  nop,  sbc,  cpx,  sbc,  inc,  isb, /* E */
  /* F */      beq,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb,  sed,  sbc,  nop,  isb,  nop,  sbc,  inc,  isb  /* F */
  };

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
  (*addrtable[opcode])();
  (*optable[opcode])();
  }
