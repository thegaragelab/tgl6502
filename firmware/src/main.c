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

#if defined(__CODE_RED)
  #include <cr_section_macros.h>
  #include <NXP/crp.h>
  __CRP const unsigned int CRP_WORD = CRP_NO_CRP ;
#endif

//----------------------------------------------------------------------------
// Hardware setup
//----------------------------------------------------------------------------

void configurePins() {
  /* Enable SWM clock */
	//  LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 7);  // this is already done in SystemInit()

  /* Pin Assign 8 bit Configuration */
  /* U0_TXD */
  /* U0_RXD */
  LPC_SWM->PINASSIGN0 = 0xffff0004UL;

  /* Pin Assign 1 bit Configuration */
  #if !defined(USE_SWD)
    /* Pin setup generated via Switch Matrix Tool
       ------------------------------------------------
       PIO0_5 = RESET
       PIO0_4 = U0_TXD
       PIO0_3 = GPIO            - Disables SWDCLK
       PIO0_2 = GPIO (User LED) - Disables SWDIO
       PIO0_1 = GPIO
       PIO0_0 = U0_RXD
       ------------------------------------------------
       NOTE: SWD is disabled to free GPIO pins!
       ------------------------------------------------ */
    LPC_SWM->PINENABLE0 = 0xffffffbfUL;
  #else
    /* Pin setup generated via Switch Matrix Tool
       ------------------------------------------------
       PIO0_5 = RESET
       PIO0_4 = U0_TXD
       PIO0_3 = SWDCLK
       PIO0_2 = SWDIO
       PIO0_1 = GPIO
       PIO0_0 = U0_RXD
       ------------------------------------------------
       NOTE: LED on PIO0_2 unavailable due to SWDIO!
       ------------------------------------------------ */
    LPC_SWM->PINENABLE0 = 0xffffffb3UL;
  #endif
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
  // TODO: Implement this
  }

//----------------------------------------------------------------------------
// Main program
//----------------------------------------------------------------------------

/** Main program
 */
int main(void) {
  // Initialise the hardware
  uartInit();
  spiInit();
  // Configure the switch matrix
  configurePins();
  // Let the EEPROM loader start
  eepromMode();
  // Go into main emulation loop
  cpuResetIO();
  cpuReset();
  while (true)
    cpuStep();
  }

