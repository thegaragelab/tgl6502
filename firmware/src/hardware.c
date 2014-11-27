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
#include <stdbool.h>
#include "cpu6502.h"
#include "hardware.h"
#include "LPC8xx.h"

//---------------------------------------------------------------------------
// Core UART access
//---------------------------------------------------------------------------

//! Baud rate to use for communication
#define BAUD_RATE 57600

//! UART control flags
#define UART_ENABLE          (1 << 0)
#define UART_DATA_LENGTH_8   (1 << 2)
#define UART_PARITY_NONE     (0 << 4)
#define UART_STOP_BIT_1      (0 << 6)

//! UART Status bits
#define UART_STATUS_RXRDY    (1 << 0)
#define UART_STATUS_RXIDLE   (1 << 1)
#define UART_STATUS_TXRDY    (1 << 2)
#define UART_STATUS_TXIDLE   (1 << 3)
#define UART_STATUS_CTSDEL   (1 << 5)
#define UART_STATUS_RXBRKDEL (1 << 11)

/** Initialise the UART interface
 *
 * Configures the UART for 57600 baud, 8N1.
 */
void uartInit() {
  /* Setup the clock and reset UART0 */
  LPC_SYSCON->UARTCLKDIV = 1;
  NVIC_DisableIRQ(UART0_IRQn);
  LPC_SYSCON->SYSAHBCLKCTRL |=  (1 << 14);
  LPC_SYSCON->PRESETCTRL    &= ~(1 << 3);
  LPC_SYSCON->PRESETCTRL    |=  (1 << 3);
  /* Configure UART0 */
  LPC_USART0->CFG = UART_DATA_LENGTH_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
  LPC_USART0->BRG = __MAIN_CLOCK / 16 / BAUD_RATE - 1;
  LPC_SYSCON->UARTFRGDIV = 0xFF;
  LPC_SYSCON->UARTFRGMULT = (((__MAIN_CLOCK / 16) * (LPC_SYSCON->UARTFRGDIV + 1)) /
    (BAUD_RATE * (LPC_USART0->BRG + 1))) - (LPC_SYSCON->UARTFRGDIV + 1);
  /* Clear the status bits */
  LPC_USART0->STAT = UART_STATUS_CTSDEL | UART_STATUS_RXBRKDEL;
  /* Enable UART0 interrupt */
//  NVIC_EnableIRQ(UART0_IRQn);
  /* Enable UART0 */
  LPC_USART0->CFG |= UART_ENABLE;
  }

/** Send a single character over the UART
 *
 * This is a blocking operation. If the transmitter is not yet ready the
 * function will block until it can send the character.
 *
 * @param ch the character to send.
 */
void uartWrite(uint8_t ch) {
  /* Wait until we're ready to send */
  while (!(LPC_USART0->STAT & UART_STATUS_TXRDY));
  LPC_USART0->TXDATA = ch;
  }

/** Send a sequence of bytes to the UART
 *
 * Sends the contents of a NUL terminated string over the UART interface.
 */
void uartWriteString(const uint8_t *cszString) {
  while(*cszString) {
    uartWrite(*cszString);
    cszString++;
    }
  }

/** Read a single character from the UART
 *
 * This is a blocking operation. If no data is available the function will
 * block until it is. Use the 'uartRead()' function to determine if data is
 * available.
 *
 * @return the character read from the UART
 */
uint8_t uartRead() {
  /* Wait until we're ready to receive */
  while (!(LPC_USART0->STAT & UART_STATUS_RXRDY));
  return LPC_USART0->RXDATA;
  }

/** Determine if data is available to read
 *
 * @return true if data is available and a call to 'uartRead()' would return
 *              immediately.
 */
bool uartAvail() {
  return (LPC_USART0->STAT & UART_STATUS_RXRDY);
  }

//---------------------------------------------------------------------------
// Core SPI access
//---------------------------------------------------------------------------

//--- Master select pin
#define MASTER_SELECT           1

//--- SPI configuration flags
#define SPI_CFG_ENABLE          (1 << 0)
#define SPI_CFG_MASTER          (1 << 2)

//--- SPI transfer flags
#define SPI_TXDATCTL_EOT        (1 << 20)
#define SPI_TXDATCTL_FSIZE(s)   ((s) << 24)
#define SPI_STAT_RXRDY          (1 << 0)
#define SPI_STAT_TXRDY          (1 << 1)

//! Enable the master (IO expander) SPI device
#define masterEnable()  LPC_GPIO_PORT->SET0 &= ~(1 << MASTER_SELECT)

//! Disable the master (IO expander) SPI device
#define masterDisable() LPC_GPIO_PORT->SET0 = 1 << MASTER_SELECT

/** Initialise SPI interface
 *
 * Set up SPI0 as master in mode 0 (CPHA and CPOL = 0) at 10MHz with no delays.
 * SSEL is not assigned to an output pin, it must be controlled separately.
 * This function also configures the pin we are using as the 'master select'
 */
void spiInit() {
  /* Enable AHB clock to the GPIO domain. */
  LPC_SYSCON->SYSAHBCLKCTRL |=  (1 << 6);
  LPC_SYSCON->PRESETCTRL    &= ~(1 << 10);
  LPC_SYSCON->PRESETCTRL    |=  (1 << 10);
  /* Set the master select to an output */
  LPC_GPIO_PORT->DIR0 |= (1 << MASTER_SELECT);
  /* Disable the master select */
  masterDisable();
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

