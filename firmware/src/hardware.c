/*--------------------------------------------------------------------------*
* TGL-6502 Hardware Emulation
*---------------------------------------------------------------------------*
* 27-Nov-2014 ShaneG
*
* This provides the hardware interface to the SPI peripheral and provides
* the emulator access to them.
*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "cpu6502.h"
#include "LPC8xx.h"

//---------------------------------------------------------------------------
// Core SPI access
//---------------------------------------------------------------------------

//--- SPI configuration flags
#define SPI_CFG_ENABLE          (1 << 0)
#define SPI_CFG_MASTER          (1 << 2)

//--- SPI transfer flags
#define SPI_TXDATCTL_EOT        (1 << 20)
#define SPI_TXDATCTL_FSIZE(s)   ((s) << 24)
#define SPI_STAT_RXRDY          (1 << 0)
#define SPI_STAT_TXRDY          (1 << 1)

/** Initialise SPI interface
 *
 * Set up SPI0 as master in mode 0 (CPHA and CPOL = 0) at 10MHz with no delays.
 * SSEL is not assigned to an output pin, it must be controlled separately.
 */
void spiInit() {
  /* Enable SPI clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);
  /* Bring SPI out of reset */
  LPC_SYSCON->PRESETCTRL &= ~(0x1<<0);
  LPC_SYSCON->PRESETCTRL |= (0x1<<0);
  /* Set clock speed and mode */
  LPC_SPI0->DIV = 3; // Use 10MHz (assuming 30MHz system clock)
  LPC_SPI0->CFG = (SPI_CFG_MASTER & ~SPI_CFG_ENABLE);
  LPC_SPI0->CFG |= SPI_CFG_ENABLE;
  }

/** Transfer a single data byte via SPI
 *
 * @param data the 8 bit value to send
 *
 * @return the data received in the transfer
 */
uint8_t spiTransfer(uint8_t data) {
  while ( (LPC_SPI0->STAT & SPI_STAT_TXRDY) == 0 );
  LPC_SPI0->TXDATCTL = SPI_TXDATCTL_FSIZE(8-1) | SPI_TXDATCTL_EOT | data;
  while ( (LPC_SPI0->STAT & SPI_STAT_RXRDY) == 0 );
  return LPC_SPI0->RXDAT;
  }

//---------------------------------------------------------------------------
// Emulator access functions
//---------------------------------------------------------------------------

/** Read a byte from the IO region
 *
 * @param address the offset into the IO area to read
 */
uint8_t cpuReadIO(uint16_t address) {
  // TODO: Implement this
  return 0;
  }

/** Write a byte to the IO region
 *
 * @param address the offset into the IO area to write
 * @param value the value to write
 */
void cpuWriteIO(uint16_t address, uint8_t byte) {
  // TODO: Implement this
  }

