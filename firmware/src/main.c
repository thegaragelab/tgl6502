/*--------------------------------------------------------------------------*
* TGL-6502 SBC Emulator
*---------------------------------------------------------------------------*
* 27-Nov-2014 ShaneG
*
* This is the main LPC810 firmware for the TGL-6502 SBC. It is very minmal
* due to the limited flash available but emulates a 6502 processor without
* the undocumented instructions.
*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "cpu6502.h"
#include "hardware.h"
#include "LPC8xx.h"

//--- Version and identification information
#define BANNER "\n\nTGL-6502"
#define HW_VERSION "RevA"
#define SW_VERSION "0.1"

//-- Guff for Code Red
#if defined(__CODE_RED)
  #include <cr_section_macros.h>
  #include <NXP/crp.h>
  __CRP const unsigned int CRP_WORD = CRP_NO_CRP ;
#endif

//----------------------------------------------------------------------------
// Hardware setup
//----------------------------------------------------------------------------

//! System ticks timer
static volatile uint32_t s_timeNow = 0;

/** Implement the system tick interrupt
 *
 */
void SysTick_Handler(void) {
  s_timeNow++;
  }

/** Determine the duration (in ms) since the last sample
 *
 * @param sample the last millisecond count
 */
uint32_t timeDuration(uint32_t sample) {
  uint32_t now = s_timeNow;
  if(sample > now)
    return now + (0xFFFFFFFFL - sample);
  return now - sample;
  }

/** Configure the switch matrix
 *
 * [PinNo.][PinName]               [Signal]    [Module]
 * 1       RESET/PIO0_5            SPI0_MISO   SPI0
 * 2       PIO0_4                  U0_TXD      USART0
 * 3       SWCLK/PIO0_3            SPI0_MOSI   SPI0
 * 4       SWDIO/PIO0_2            SPI0_SCK    SPI0
 * 5       PIO0_1/ACMP_I2/CLKIN    PIO0_1      GPIO0
 * 6       VDD
 * 7       VSS
 * 8       PIO0_0/ACMP_I1          U0_RXD      USART0
 */
void configurePins() {
  /* Enable SWM clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);
  /* Pin Assign 8 bit Configuration */
  /* U0_TXD */
  /* U0_RXD */
  LPC_SWM->PINASSIGN0 = 0xffff0004UL;
  /* SPI0_SCK */
  LPC_SWM->PINASSIGN3 = 0x02ffffffUL;
  /* SPI0_MOSI */
  /* SPI0_MISO */
  LPC_SWM->PINASSIGN4 = 0xffff0503UL;
  /* Pin Assign 1 bit Configuration */
  LPC_SWM->PINENABLE0 = 0xffffffffUL;
  /* Enable UART clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<18);
  // Set up the system tick timer
  SysTick->LOAD = (SystemCoreClock / 1000 * 1/*# ms*/) - 1; /* Every 1 ms */
  SysTick->VAL = 0;
  SysTick->CTRL = 0x7; /* d2:ClkSrc=SystemCoreClock, d1:Interrupt=enabled, d0:SysTick=enabled */
  }

//----------------------------------------------------------------------------
// EEPROM loader mode implementation
//----------------------------------------------------------------------------

/** EEPROM loader entry point
 *
 * This allows a short wait (about 3s) to receive a start character to trigger
 * the EEPROM loader mode. If that is not received we simply drop out and let
 * the emulator start.
 */
static void eepromMode() {
  // Wait for up to 3 seconds for the entry byte (0x55)
  uint8_t ch;
  uint32_t start = s_timeNow;
  while(timeDuration(start)<3000) {
    if(uartAvail()) {
      ch = uartRead();
      if(ch==0x55)
        break;
      return;
      }
    }
  if(timeDuration(start)>=3000)
    return;
  // Go into loading mode
  uartWrite('+');
  uint16_t addr, index, checksum;
  uint8_t *pData = (uint8_t *)&g_ioState;
  while(true) {
    ch = uartRead();
    // Every command has a two byte address
    addr = uartRead();
    addr = (addr << 8) | uartRead();
    // Process according to command
    if(ch=='R') {
      // Read the page
      eepromReadPage(addr, pData);
      // Return it
      uartWrite('+');
      for(index=0; index<EEPROM_PAGE_SIZE; index++)
        uartWrite(pData[index]);
      }
    else if(ch=='W') {
      checksum = 0;
      index = 0;
      while(index<EEPROM_PAGE_SIZE) {
        pData[index] = uartRead();
        checksum += pData[index];
        }
      // Get the checksum and check it
      index = uartRead();
      index = (index << 8) | uartRead();
      if(index!=checksum)
        uartWrite('-');
      else {
        eepromWritePage(addr, pData);
        uartWrite('+');
        }
      }
    }
  }

//----------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------

/** Main program
 */
int main(void) {
  // Configure the switch matrix
  configurePins();
  // Initialise the hardware
  uartInit();
  spiInit();
  hwInit();
  // Show the banner
  uartWriteString(BANNER " " HW_VERSION "/" SW_VERSION "\n");
  // Let the EEPROM loader start
  eepromMode();
  // Go into main emulation loop
  uint32_t now = s_timeNow;
  while(true) {
    // Show something every second
    if(timeDuration(now)>=1000) {
      now = s_timeNow;
      uartWriteString("Beep!\n");
      hwInit();
      }
    // Check for input
    if(uartAvail()) {
      uint8_t ch = uartRead();
      if((ch>='a')&&(ch<='z'))
        ch = (ch - 'a') + 'A';
      uartWrite(ch);
      }
    }
/*
  cpuResetIO();
  cpuReset();
  while (true)
    cpuStep();
*/
  }

