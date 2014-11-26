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

//addressing mode functions, calculates effective addresses
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
static void adc() {
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

/** Apply the addressing mode
 *
 * This is very ugly and inefficient but requires far less code space than a
 * table of function pointers.
 */
static void applyMode(uint8_t opcode) {
  switch(opcode) {
    case 0x0c:
    case 0x0d:
    case 0x0e:
    case 0x0f:
    case 0x20:
    case 0x2c:
    case 0x2d:
    case 0x2e:
    case 0x2f:
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:
    case 0x6d:
    case 0x6e:
    case 0x6f:
    case 0x8c:
    case 0x8d:
    case 0x8e:
    case 0x8f:
    case 0xac:
    case 0xad:
    case 0xae:
    case 0xaf:
    case 0xcc:
    case 0xcd:
    case 0xce:
    case 0xcf:
    case 0xec:
    case 0xed:
    case 0xee:
    case 0xef:
      abso();
      break;
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
    case 0x5c:
    case 0x5d:
    case 0x5e:
    case 0x5f:
    case 0x7c:
    case 0x7d:
    case 0x7e:
    case 0x7f:
    case 0x9c:
    case 0x9d:
    case 0xbc:
    case 0xbd:
    case 0xdc:
    case 0xdd:
    case 0xde:
    case 0xdf:
    case 0xfc:
    case 0xfd:
    case 0xfe:
    case 0xff:
      absx();
      break;
    case 0x19:
    case 0x1b:
    case 0x39:
    case 0x3b:
    case 0x59:
    case 0x5b:
    case 0x79:
    case 0x7b:
    case 0x99:
    case 0x9b:
    case 0x9e:
    case 0x9f:
    case 0xb9:
    case 0xbb:
    case 0xbe:
    case 0xbf:
    case 0xd9:
    case 0xdb:
    case 0xf9:
    case 0xfb:
      absy();
      break;
    case 0x09:
    case 0x0b:
    case 0x29:
    case 0x2b:
    case 0x49:
    case 0x4b:
    case 0x69:
    case 0x6b:
    case 0x80:
    case 0x82:
    case 0x89:
    case 0x8b:
    case 0xa0:
    case 0xa2:
    case 0xa9:
    case 0xab:
    case 0xc0:
    case 0xc2:
    case 0xc9:
    case 0xcb:
    case 0xe0:
    case 0xe2:
    case 0xe9:
    case 0xeb:
      imm();
      break;
    case 0x6c:
      ind();
      break;
    case 0x01:
    case 0x03:
    case 0x21:
    case 0x23:
    case 0x41:
    case 0x43:
    case 0x61:
    case 0x63:
    case 0x81:
    case 0x83:
    case 0xa1:
    case 0xa3:
    case 0xc1:
    case 0xc3:
    case 0xe1:
    case 0xe3:
      indx();
      break;
    case 0x11:
    case 0x13:
    case 0x31:
    case 0x33:
    case 0x51:
    case 0x53:
    case 0x71:
    case 0x73:
    case 0x91:
    case 0x93:
    case 0xb1:
    case 0xb3:
    case 0xd1:
    case 0xd3:
    case 0xf1:
    case 0xf3:
      indy();
      break;
    case 0x10:
    case 0x30:
    case 0x50:
    case 0x70:
    case 0x90:
    case 0xb0:
    case 0xd0:
    case 0xf0:
      rel();
      break;
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0xa4:
    case 0xa5:
    case 0xa6:
    case 0xa7:
    case 0xc4:
    case 0xc5:
    case 0xc6:
    case 0xc7:
    case 0xe4:
    case 0xe5:
    case 0xe6:
    case 0xe7:
      zp();
      break;
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x94:
    case 0x95:
    case 0xb4:
    case 0xb5:
    case 0xd4:
    case 0xd5:
    case 0xd6:
    case 0xd7:
    case 0xf4:
    case 0xf5:
    case 0xf6:
    case 0xf7:
      zpx();
      break;
    case 0x96:
    case 0x97:
    case 0xb6:
    case 0xb7:
      zpy();
      break;
    }
  }

/** Apply the opcode
 *
 * This is very ugly and inefficient but requires far less code space than a
 * table of function pointers.
 */
static void applyOpcode(uint8_t opcode) {
  switch(opcode) {
    case 0x61:
    case 0x65:
    case 0x69:
    case 0x6d:
    case 0x71:
    case 0x75:
    case 0x79:
    case 0x7d:
      adc();
      break;
    case 0x21:
    case 0x25:
    case 0x29:
    case 0x2d:
    case 0x31:
    case 0x35:
    case 0x39:
    case 0x3d:
      and();
      break;
    case 0x06:
    case 0x0a:
    case 0x0e:
    case 0x16:
    case 0x1e:
      asl();
      break;
    case 0x90:
      bcc();
      break;
    case 0xb0:
      bcs();
      break;
    case 0xf0:
      beq();
      break;
    case 0x24:
    case 0x2c:
      bit();
      break;
    case 0x30:
      bmi();
      break;
    case 0xd0:
      bne();
      break;
    case 0x10:
      bpl();
      break;
    case 0x00:
      brk();
      break;
    case 0x50:
      bvc();
      break;
    case 0x70:
      bvs();
      break;
    case 0x18:
      clc();
      break;
    case 0xd8:
      cld();
      break;
    case 0x58:
      cli();
      break;
    case 0xb8:
      clv();
      break;
    case 0xc1:
    case 0xc5:
    case 0xc9:
    case 0xcd:
    case 0xd1:
    case 0xd5:
    case 0xd9:
    case 0xdd:
      cmp();
      break;
    case 0xe0:
    case 0xe4:
    case 0xec:
      cpx();
      break;
    case 0xc0:
    case 0xc4:
    case 0xcc:
      cpy();
      break;
    case 0xc3:
    case 0xc7:
    case 0xcf:
    case 0xd3:
    case 0xd7:
    case 0xdb:
    case 0xdf:
      dcp();
      break;
    case 0xc6:
    case 0xce:
    case 0xd6:
    case 0xde:
      dec();
      break;
    case 0xca:
      dex();
      break;
    case 0x88:
      dey();
      break;
    case 0x41:
    case 0x45:
    case 0x49:
    case 0x4d:
    case 0x51:
    case 0x55:
    case 0x59:
    case 0x5d:
      eor();
      break;
    case 0xe6:
    case 0xee:
    case 0xf6:
    case 0xfe:
      inc();
      break;
    case 0xe8:
      inx();
      break;
    case 0xc8:
      iny();
      break;
    case 0xe3:
    case 0xe7:
    case 0xef:
    case 0xf3:
    case 0xf7:
    case 0xfb:
    case 0xff:
      isb();
      break;
    case 0x4c:
    case 0x6c:
      jmp();
      break;
    case 0x20:
      jsr();
      break;
    case 0xa3:
    case 0xa7:
    case 0xaf:
    case 0xb3:
    case 0xb7:
    case 0xbb:
    case 0xbf:
      lax();
      break;
    case 0xa1:
    case 0xa5:
    case 0xa9:
    case 0xad:
    case 0xb1:
    case 0xb5:
    case 0xb9:
    case 0xbd:
      lda();
      break;
    case 0xa2:
    case 0xa6:
    case 0xae:
    case 0xb6:
    case 0xbe:
      ldx();
      break;
    case 0xa0:
    case 0xa4:
    case 0xac:
    case 0xb4:
    case 0xbc:
      ldy();
      break;
    case 0x46:
    case 0x4a:
    case 0x4e:
    case 0x56:
    case 0x5e:
      lsr();
      break;
    case 0x01:
    case 0x05:
    case 0x09:
    case 0x0d:
    case 0x11:
    case 0x15:
    case 0x19:
    case 0x1d:
      ora();
      break;
    case 0x48:
      pha();
      break;
    case 0x08:
      php();
      break;
    case 0x68:
      pla();
      break;
    case 0x28:
      plp();
      break;
    case 0x23:
    case 0x27:
    case 0x2f:
    case 0x33:
    case 0x37:
    case 0x3b:
    case 0x3f:
      rla();
      break;
    case 0x26:
    case 0x2a:
    case 0x2e:
    case 0x36:
    case 0x3e:
      rol();
      break;
    case 0x66:
    case 0x6a:
    case 0x6e:
    case 0x76:
    case 0x7e:
      ror();
      break;
    case 0x63:
    case 0x67:
    case 0x6f:
    case 0x73:
    case 0x77:
    case 0x7b:
    case 0x7f:
      rra();
      break;
    case 0x40:
      rti();
      break;
    case 0x60:
      rts();
      break;
    case 0x83:
    case 0x87:
    case 0x8f:
    case 0x97:
      sax();
      break;
    case 0xe1:
    case 0xe5:
    case 0xe9:
    case 0xeb:
    case 0xed:
    case 0xf1:
    case 0xf5:
    case 0xf9:
    case 0xfd:
      sbc();
      break;
    case 0x38:
      sec();
      break;
    case 0xf8:
      sed();
      break;
    case 0x78:
      sei();
      break;
    case 0x03:
    case 0x07:
    case 0x0f:
    case 0x13:
    case 0x17:
    case 0x1b:
    case 0x1f:
      slo();
      break;
    case 0x43:
    case 0x47:
    case 0x4f:
    case 0x53:
    case 0x57:
    case 0x5b:
    case 0x5f:
      sre();
      break;
    case 0x81:
    case 0x85:
    case 0x8d:
    case 0x91:
    case 0x95:
    case 0x99:
    case 0x9d:
      sta();
      break;
    case 0x86:
    case 0x8e:
    case 0x96:
      stx();
      break;
    case 0x84:
    case 0x8c:
    case 0x94:
      sty();
      break;
    case 0xaa:
      tax();
      break;
    case 0xa8:
      tay();
      break;
    case 0xba:
      tsx();
      break;
    case 0x8a:
      txa();
      break;
    case 0x9a:
      txs();
      break;
    case 0x98:
      tya();
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

void cpuStep() {
  opcode = cpuReadByte(g_cpuState.m_pc++);
  applyMode(opcode);
  applyOpcode(opcode);
  }
