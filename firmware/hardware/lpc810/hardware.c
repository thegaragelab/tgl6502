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
#include <tgl6502.h>
#include "LPC8xx.h"

//! Size of memory (both RAM and ROM)
#define MEMORY_SIZE (128 * 1024)

//! IO state information
IO_STATE g_ioState;

//---------------------------------------------------------------------------
// Version information
//
// The version byte indicates the hardware platform in the upper nybble and
// the firmware version in the lower. Available platforms are:
//
//  0 - The desktop simulator.
//  1 - LPC810 based RevA hardware
//  2 - LPC810 based RevB hardware
//---------------------------------------------------------------------------

// This files supports RevA and RevB boards so the HW version may be defined
// as part of the build process. If not assume a RevA board.
#if !defined(HW_VERSION)
#  define HW_VERSION 1
#endif
#define FW_VERSION 1

// The single version byte (readable from the IO block)
#define VERSION_BYTE ((HW_VERSION << 4) | FW_VERSION)

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

/** Determine if a quarter second has elapsed.
 *
 * @return true if 0.25 seconds has passed.
 */
bool qsecElapsed() {
  if(s_timeNow>=250) {
    s_timeNow = 0;
    return true;
    }
  return false;
  }

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
#define SPI_CFG_CPHA            (1 << 4)

//--- SPI transfer flags
#define SPI_TXDATCTL_EOT        (1 << 20)
#define SPI_TXDATCTL_FSIZE(s)   ((s) << 24)
#define SPI_STAT_RXRDY          (1 << 0)
#define SPI_STAT_TXRDY          (1 << 1)

//! Enable the master (IO expander) SPI device
#define masterEnable()  LPC_GPIO_PORT->B0[1] = 0

//! Disable the master (IO expander) SPI device
#define masterDisable() LPC_GPIO_PORT->B0[1] = 1

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

//--- Read or write operation (IO Expander)
#define IO_READ  0x47
#define IO_WRITE 0x46

/** Register definitions for the IO expander (MCP23S08)
 */
typedef enum {
  IODIR = 0, //!< Direction register
  IPOL,      //!< Polarity register
  GPINTEN,   //!< Interrupt on change
  DEFVAL,    //!< Default compare value
  INTCON,    //!< Interrupt control
  IOCON,     //!< Module control
  GPPU,      //!< Pull up resistor control
  INTF,      //!< Interrupt flag register
  INTCAP,    //!< Interrupt capture
  GPIO,      //!< GPIO
  OLAT,      //!< Output latch
  } IO_REGISTER;

/** Bit values and masks for the GPIO port
 */
typedef enum {
  GPIO_SS0     = 0x02, //!< Slave select 0
  GPIO_SS1     = 0x01, //!< Slave select 1
  GPIO_SS2     = 0x04, //!< Slave select 2
  GPIO_SS_MASK = 0x07, //!< Mask for slave select bits
  GPIO_SLD0    = 0x08, //!< D0 on slot
  GPIO_SLD1    = 0x10, //!< D1 on slot
  GPIO_SL_MASK = 0x18, //!< Mask for slot IO
  GPIO_LED1    = 0x20, //!< Front panel LED 1
  GPIO_LED0    = 0x40, //!< Front panel LED 0
  GPIO_BTN     = 0x80, //!< Front panel button
  } GPIO_PORT;

/** The external SPI devices available
 */
typedef enum {
  SPI_NONE   = 0,        //!< Deselect all devices
  SPI_EEPROM = GPIO_SS0, //!< The EEPROM chip
  SPI_RAM    = GPIO_SS1, //!< SRAM chip
  SPI_SLOT   = GPIO_SS2, //!< Expansion slot 0
  } SPI_DEVICE;


/** The control registers we are interested in
 */
typedef struct _GPIO_REG {
  uint8_t m_iodir;
  uint8_t m_gpio;
  uint8_t m_pullup;
  } GPIO_REG;

//! Current values of IO expander registers
static GPIO_REG s_regsActive;

//! Desired value s of IO expander registers
static GPIO_REG s_regsDesired;

/** Read or write a register on the IO expander
 */
static uint8_t ioRegister(uint8_t op, uint8_t reg, uint8_t data) {
  masterEnable();
  spiTransfer(op);
  spiTransfer(reg);
  uint8_t result = spiTransfer(data);
  masterDisable();
  return result;
  }

/** Update the IO expander registers
 *
 * We keep a copy of the desired register settings so we can batch changes
 * across different functions. Updates are only done if something has changed
 * which avoids unnecessary SPI transactions.
 */
static void ioUpdateRegisters() {
  if(s_regsDesired.m_iodir!=s_regsActive.m_iodir) {
    ioRegister(IO_WRITE, IODIR, s_regsDesired.m_iodir);
    s_regsActive.m_iodir = s_regsDesired.m_iodir;
    }
  if(s_regsDesired.m_gpio!=s_regsActive.m_gpio) {
    ioRegister(IO_WRITE, GPIO, s_regsDesired.m_gpio);
    s_regsActive.m_gpio = s_regsDesired.m_gpio;
    }
  if(s_regsDesired.m_pullup!=s_regsActive.m_pullup) {
    ioRegister(IO_WRITE, GPPU, s_regsDesired.m_pullup);
    s_regsActive.m_pullup = s_regsDesired.m_pullup;
    }
  }

/** Select a peripheral device
 *
 * Change the outputs on the IO expander to select one of the additional SPI
 * devices. The selection will be active when the function returns.
 *
 * @param device the peripheral to select.
 */
static void selectDevice(SPI_DEVICE device) {
  s_regsDesired.m_gpio = (s_regsDesired.m_gpio & ~GPIO_SS_MASK) | device;
  ioUpdateRegisters();
  }

/** Perform a read or write operation on one of the memory chips
 *
 * @param cmd the command to perform
 * @param phys the 'physical' address (21 bits, MSB indicates ROM/RAM)
 * @param value the value to write for a write operation
 *
 * @return the value read for a read operation.
 */
static uint8_t memReadWrite(uint8_t cmd, uint32_t phys, uint8_t value) {
  // Read or write memory
  uint8_t result = 0;
  selectDevice((phys&ROM_BASE)?SPI_EEPROM:SPI_RAM);
  phys &= 0x0007FFFL;
  if (phys<MEMORY_SIZE) {
    spiTransfer(cmd);
    spiTransfer((uint8_t)((phys >> 16) & 0xFF));
    spiTransfer((uint8_t)((phys >> 8) & 0xFF));
    spiTransfer((uint8_t)(phys & 0xFF));
    result = spiTransfer(value);
    }
  selectDevice(SPI_NONE);
  return result;
  }

//---------------------------------------------------------------------------
// Memory access
//---------------------------------------------------------------------------

/** SPI memory chip commands
 */
typedef enum {
  MEMORY_WRITE  = 0x02, //! Write data to memory
  MEMORY_READ   = 0x03, //! Read data from memory
  MEMORY_STATUS = 0x05, //! Read status (EEPROM only)
  MEMORY_WREN   = 0x06, //! Enable writes (EEPROM only)
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
  // Check for overflow
  if(address>=sizeof(IO_STATE))
    return 0xFF;
  switch(address) {
    case IO_OFFSET_CONIN:
      return uartRead();
    case IO_OFFSET_IOCTL:
    case IO_OFFSET_SLOT0:
      // Read current IO
      if(uartAvail())
        g_ioState.m_ioctl |= IOCTL_CONDATA;
      else
        g_ioState.m_ioctl &= ~IOCTL_CONDATA;
      break;
    }
  return ((uint8_t *)&g_ioState)[address];
  }

/** Write a byte to the IO region
 *
 * @param address the offset into the IO area to write
 * @param value the value to write
 */
static void cpuWriteIO(uint16_t address, uint8_t byte) {
  // Check for overflow
  if(address>=sizeof(IO_STATE))
    return;
  switch(address) {
    case IO_OFFSET_VERSION:
    case IO_OFFSET_CONIN:
    case IO_OFFSET_KIPS:
    case IO_OFFSET_KIPS + 1:
      // Read only entries
      return;
    case IO_OFFSET_CONOUT:
      uartWrite(byte);
      return;
    }
  // Update the value
  ((uint8_t *)&g_ioState)[address] = byte;
  switch(address) {
    case IO_OFFSET_IOCTL:
    case IO_OFFSET_SLOT0:
      // Update IO state
      break;
    case IO_OFFSET_SPICMD:
      // Do the SPI transaction
      break;
    }
  }

//---------------------------------------------------------------------------
// Emulator access functions
//---------------------------------------------------------------------------

//--- Predefined register values to avoid bringing in division functions
#define UARTFRGDIV_VAL 0xff
#define USART_BRG_VAL __MAIN_CLOCK / 16 / BAUD_RATE - 1

/** Initialise the hardware
 *
 * In this function we set up the switch matrix, initialise the GPIO, UART
 * and SPI modules and finally set up the IO expander.
 *
 * Switch matrix configuration
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
void hwInit() {
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
  SysTick->LOAD = (__SYSTEM_CLOCK / 1000 * 1) - 1; /* Every 1 ms */
  SysTick->VAL = 0;
  SysTick->CTRL = 0x7; /* d2:ClkSrc=SystemCoreClock, d1:Interrupt=enabled, d0:SysTick=enabled */
  // Set up the UART
  /* Setup the clock and reset UART0 */
  LPC_SYSCON->UARTCLKDIV = 1;
  NVIC_DisableIRQ(UART0_IRQn);
  LPC_SYSCON->SYSAHBCLKCTRL |=  (1 << 14);
  LPC_SYSCON->PRESETCTRL    &= ~(1 << 3);
  LPC_SYSCON->PRESETCTRL    |=  (1 << 3);
  /* Configure UART0 */
  LPC_USART0->CFG = UART_DATA_LENGTH_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
  LPC_USART0->BRG = USART_BRG_VAL;
  LPC_SYSCON->UARTFRGDIV = UARTFRGDIV_VAL;
  LPC_SYSCON->UARTFRGMULT = (((__MAIN_CLOCK / 16) * (UARTFRGDIV_VAL + 1)) /
    (BAUD_RATE * (USART_BRG_VAL + 1))) - (UARTFRGDIV_VAL + 1);
  /* Clear the status bits */
  LPC_USART0->STAT = UART_STATUS_CTSDEL | UART_STATUS_RXBRKDEL;
  /* Enable UART0 interrupt */
//  NVIC_EnableIRQ(UART0_IRQn);
  /* Enable UART0 */
  LPC_USART0->CFG |= UART_ENABLE;
  // Set up SPI module
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
  LPC_SPI0->DIV = 14; // Use 2MHz (assuming 30MHz system clock)
  LPC_SPI0->CFG = SPI_CFG_MASTER;
  LPC_SPI0->CFG |= SPI_CFG_ENABLE;
  // Set up the IO expander
  s_regsDesired.m_iodir = GPIO_BTN;
  s_regsDesired.m_gpio = 0;
  s_regsDesired.m_pullup = GPIO_BTN;
  ioUpdateRegisters();
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
  // Stash the version information
  g_ioState.m_version = VERSION_BYTE;
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
  return memReadWrite(MEMORY_READ, getPhysicalAddress(address), 0);
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
    memReadWrite(MEMORY_WRITE, phys, value);
    }
  }

