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

//! IO state information
IO_STATE g_ioState;

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
void uartWriteString(const char *cszString) {
  while(*cszString) {
    uartWrite((uint8_t)*cszString);
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
// Peripheral access helpers
//---------------------------------------------------------------------------

/** The external SPI devices available
 */
typedef enum {
  SPI_EEPROM = 0, //!< The EEPROM chip
  SPI_RAM,        //!< SRAM chip
  SPI_SLOT0,      //!< Expansion slot 0
  SPI_SLOT1,      //!< Expansion slot 1
  SPI_NONE,       //!< Deselect all devices
  } SPI_DEVICE;

//--- Read or write operation (IO Expander)
#define IO_READ  0x41
#define IO_WRITE 0x40

/** Register definitions for the IO expander (MCP23S17)
 */
typedef enum {
  IODIRA = 0,  //!< Direction register, Port A
  IODIRB,
  IPOLA,       //!< Polarity register, Port A
  IPOLB,
  GPINTENA,    //!< Interrupt on change, Port A
  GPINTENB,
  DEFVALA,     //!< Default compare value, Port A
  DEFVALB,
  INTCONA,     //!< Interrupt control, Port A
  INTCONB,
  IOCON,       //!< Module control
  IOCON_,      //!< Mirror of above
  GPPUA,       //!< Pull up resistor control
  GPPUB,
  INTFA,       //!< Interrupt flag register, Port A
  INTFB,
  INTCAPA,     //!< Interrupt capture, Port A
  INTCAPB,
  GPIOA,       //!< GPIO, Port A
  GPIOB,
  OLATA,       //!< Output latch, Port A
  OLATB,
  } IO_REGISTER;

/** Read a register on the IO expander
 */
static uint8_t ioReadRegister(uint8_t reg) {
  masterEnable();
  spiTransfer(IO_READ);
  spiTransfer(reg);
  uint8_t result = spiTransfer(0);
  masterDisable();
  return result;
  }

/** Write a register on the IO expander
 */
static void ioWriteRegister(uint8_t reg, uint8_t value) {
  masterEnable();
  spiTransfer(IO_READ);
  spiTransfer(reg);
  spiTransfer(value);
  masterDisable();
  }

/** Select a peripheral device
 *
 * Change the outputs on the IO expander to select one of the additional SPI
 * devices. The selection will be active when the function returns.
 *
 * @param device the peripheral to select.
 */
static void selectDevice(SPI_DEVICE device) {
  // TODO: Implement this
  }

//---------------------------------------------------------------------------
// Memory access
//---------------------------------------------------------------------------

//! Size of memory (using same size for ROM and RAM)
#define MEMORY_SIZE (128 * 1024)

//! The start of ROM space in physical memory
#define ROM_BASE 0x00080000L

//! The start of IO space in CPU memory
#define IO_BASE 0xFF00

/** SPI memory chip commands
 */
typedef enum {
  MEMORY_WRITE = 0x02, //! Write data to memory
  MEMORY_READ  = 0x03, //! Read data from memory
  MEMORY_WREN  = 0x06, //! Enable writes (EEPROM only)
  } MEMORY_COMMAND;

/** Calculate the physical address with the current page settings.
 *
 * @param address the 16 bit CPU address
 *
 * @return the 32 bit (20 used) physical address.
 */
static uint32_t getPhysicalAddress(uint16_t address) {
  return ((uint32_t)address & 0x1FFF) | ((uint32_t)(g_ioState.m_pages[address >> 13] & 0x7F) << 13);
  }

/** Read a byte from the IO region
 *
 * @param address the offset into the IO area to read
 */
static uint8_t cpuReadIO(uint16_t address) {
  // TODO: Implement this
  return 0;
  }

/** Write a byte to the IO region
 *
 * @param address the offset into the IO area to write
 * @param value the value to write
 */
static void cpuWriteIO(uint16_t address, uint8_t byte) {
  // TODO: Implement this
  }

//---------------------------------------------------------------------------
// Emulator access functions
//---------------------------------------------------------------------------

/** Initialise the external hardware
 *
 * This function needs to be called after the SPI bus has been initialised and
 * the pin switch matrix has been set up. It will initialise the external SPI
 * components.
 */
void hwInit() {
  // TODO: Set up the IO expander
  // Set the SRAM chip (23LC1024) into byte transfer mode
  selectDevice(SPI_RAM);
  spiTransfer(0x01); // Write mode register
  spiTransfer(0x00); // Byte mode
  selectDevice(SPI_NONE);
  }

/** Initialise the memory and IO subsystem
 *
 * This function should be called every time the 'cpuReset()' function is
 * called. In ensures the memory mapped IO region is simulating the 'power on'
 * state.
 */
void cpuResetIO() {
  uint8_t index, *pIO = (uint8_t *)&g_ioState;
  // Clear the entire IO space
  for(index=0; index<sizeof(IO_STATE); index++)
    pIO[index] = 0;
  // Set up the default memory mapping
  for(index=0; index<6; index++)
    g_ioState.m_pages[index] = index;
  g_ioState.m_pages[6] = 0x40; // ROM page 0
  g_ioState.m_pages[7] = 0x41; // ROM page 1
  }

/** Read a single byte from the CPU address space.
 *
 * @param address the 16 bit address to read a value from.
 *
 * @return the byte read from the address.
 */
uint8_t cpuReadByte(uint16_t address) {
  // Check for IO reads
  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
    return cpuReadIO(address - IO_BASE);
  // Read from memory
  uint8_t result = 0;
  uint32_t phys = getPhysicalAddress(address);
  selectDevice((phys&ROM_BASE)?SPI_EEPROM:SPI_RAM);
  phys &= 0x0007FFFL;
  if (phys<MEMORY_SIZE) {
    spiTransfer(MEMORY_READ);
    spiTransfer((uint8_t)((phys >> 16) & 0xFF));
    spiTransfer((uint8_t)((phys >> 8) & 0xFF));
    spiTransfer((uint8_t)(phys & 0xFF));
    result = spiTransfer(0);
    }
  selectDevice(SPI_NONE);
  return result;
  }

/** Write a single byte to the CPU address space.
 *
 * @param address the 16 bit address to write a value to
 */
void cpuWriteByte(uint16_t address, uint8_t value) {
  // Check for IO writes
  if ((address>=IO_BASE)&&(address<(IO_BASE + sizeof(IO_STATE))))
    cpuWriteIO(address - IO_BASE, value);
  else {
    // Write data to RAM only
    uint32_t phys = getPhysicalAddress(address);
    if (phys>MEMORY_SIZE)
      return; // Out of RAM range
    selectDevice(SPI_RAM);
    spiTransfer(MEMORY_WRITE);
    spiTransfer((uint8_t)((phys >> 16) & 0xFF));
    spiTransfer((uint8_t)((phys >> 8) & 0xFF));
    spiTransfer((uint8_t)(phys & 0xFF));
    spiTransfer(value);
    selectDevice(SPI_NONE);
    }
  }

/** Read a page of data from the EEPROM
 *
 * Fill a buffer with data from the selected page. The buffer must be large
 * enough to hold the page (@see EEPROM_PAGE_SIZE).
 *
 * TODO: The SPI read sequence is now used in two places, break out into a
 *       separate function?
 *
 * @param address the page address to read
 * @param pData pointer to the buffer to hold the data.
 */
void eepromReadPage(uint16_t address, uint8_t *pData) {
  uint32_t phys = (uint32_t)address << 5; // Turn page into physical
  selectDevice(SPI_EEPROM);
  spiTransfer(MEMORY_READ);
  spiTransfer((uint8_t)((phys >> 16) & 0xFF));
  spiTransfer((uint8_t)((phys >> 8) & 0xFF));
  spiTransfer((uint8_t)(phys & 0xFF));
  uint8_t index;
  for(index=0; index<EEPROM_PAGE_SIZE; index++)
    pData[index] = spiTransfer(0);
  selectDevice(SPI_NONE);
  }

/** Write a page of data to the EEPROM
 *
 * Write a buffer of data to the selected page. The buffer must be large
 * enough to hold the page (@see EEPROM_PAGE_SIZE).
 *
 * @param address the page address to write
 * @param pData pointer to the buffer holding the data.
 */
void eepromWritePage(uint16_t address, uint8_t *pData) {
  uint32_t phys = (uint32_t)address << 5; // Turn page into physical
  // Enable write operations
  selectDevice(SPI_EEPROM);
  spiTransfer(MEMORY_WREN);
  selectDevice(SPI_NONE);
  // Transfer the data
  selectDevice(SPI_EEPROM);
  spiTransfer(MEMORY_WRITE);
  spiTransfer((uint8_t)((phys >> 16) & 0xFF));
  spiTransfer((uint8_t)((phys >> 8) & 0xFF));
  spiTransfer((uint8_t)(phys & 0xFF));
  uint8_t index;
  for(index=0; index<EEPROM_PAGE_SIZE; index++)
    spiTransfer(pData[index]);
  selectDevice(SPI_NONE);
  // TODO: Should check status to wait for write to complete
  }

