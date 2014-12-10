/*--------------------------------------------------------------------------*
* TGL-6502 Hardware Emulation
*---------------------------------------------------------------------------*
* 27-Nov-2014 ShaneG
*
* This provides the hardware interface to the SPI peripheral and provides
* the emulator access to them.
*--------------------------------------------------------------------------*/
#ifndef __HARDWARE_H
#define __HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

//! Size of memory (using same size for ROM and RAM)
#define MEMORY_SIZE (128 * 1024)

//! The start of ROM space in physical memory
#define ROM_BASE 0x00080000L

//! The start of IO space in CPU memory
#define IO_BASE 0xFF00

//! Size of EEPROM page (TODO: Can be up to 256, the larger the better)
#define EEPROM_PAGE_SIZE 32

#pragma pack(push, 1)

/** Represents the memory mapped IO area
 *
 * This structure maintains data for the memory mapped IO area.
 */
typedef struct _IO_STATE {
  uint16_t    m_time;                    //!< RW: Current time (seconds since midnight)
  uint16_t    m_kips;                    //!< RO: Instructions per second (1000s)
  uint8_t     m_ioctl;                   //!< RW: IO control flags
  uint8_t     m_conin;                   //!< RO: Console input
  uint8_t     m_conout;                  //!< RW: Console output
  uint8_t     m_slot0;                   //!< RW: Expansion slot control
  uint8_t     m_spicmd;                  //!< RW: SPI control byte
  uint8_t     m_spibuf[SPI_BUFFER_SIZE]; //!< RW: SPI transfer buffer
  uint8_t     m_pages[MMU_PAGE_COUNT];   //!< RW: MMU page map
  } IO_STATE;

#pragma pack(pop)

/** IO Offsets
 *
 * Offset into the IO area for each entry. This *MUST* match the IO_STATE
 * structure.
 */
typedef enum {
  IO_OFFSET_TIME  = 0,
  IO_OFFSET_KIPS  = 2,
  IO_OFFSET_IOCTL = 4,
  IO_OFFSET_CONIN = 5,
  IO_OFFSET_CONOUT = 6,
  IO_OFFSET_SLOT0 = 7,
  IO_OFFSET_SPICMD = 8,
  IO_OFFSET_SPIBUFF = 9,
  IO_OFFSET_PAGES = 9 + SPI_BUFFER_SIZE,
  } IO_OFFSET;

/** IO control bits and masks
 */
typedef enum {
  IOCTL_LED0      = 0x01, //!< Control LED0 state
  IOCTL_LED1      = 0x02, //!< Control LED1 state
  IOCTL_CONDATA   = 0x04, //!< Data is available to read from the console
  IOCTL_BUTTON    = 0x08, //!< Front panel button state
  //--- IRQ source
  IOCTL_IRQ_MASK  = 0x30, //!< Mask for IRQ source control
  IOCTL_IRQ_NONE  = 0x00, //!< IRQ disabled
  IOCTL_IRQ_TIMER = 0x10, //!< IRQ triggered by timer
  IOCTL_IRQ_INPUT = 0x20, //!< IRQ triggered by console input
  IOCTL_IRQ_SLOT  = 0x30, //!< IRQ triggered by slot D0
  //--- NMI source
  IOCTL_NMI_MASK  = 0xC0, //!< Mask for NMI source control
  IOCTL_NMI_NONE  = 0x00, //!< NMI disabled
  IOCTL_NMI_TIMER = 0x40, //!< NMI triggered by timer
  IOCTL_NMI_INPUT = 0x80, //!< NMI triggered by console input
  IOCTL_NMI_SLOT  = 0xC0, //!< NMI triggered by slot D0
  } IOCTL_FLAGS;

/** Slot 0 control bits and masks
 */
typedef enum {
  SLOT_D0_VAL = 0x01, //!< Value of pin D0
  SLOT_D1_VAL = 0x02, //!< Value of pin D1
  SLOT_D0_DIR = 0x04, //!< Direction of pin D0
  SLOT_D1_DIR = 0x08, //!< Direction of pin D1
  } SLOT_FLAGS;

//! The global IO state information
extern IO_STATE g_ioState;

/** Initialise the UART interface
 *
 * Configures the UART for 57600 baud, 8N1.
 */
void uartInit();

/** Send a single character over the UART
 *
 * This is a blocking operation. If the transmitter is not yet ready the
 * function will block until it can send the character.
 *
 * @param ch the character to send.
 */
void uartWrite(uint8_t ch);

/** Send a sequence of bytes to the UART
 *
 * Sends the contents of a NUL terminated string over the UART interface.
 */
void uartWriteString(const char *cszString);

/** Read a single character from the UART
 *
 * This is a blocking operation. If no data is available the function will
 * block until it is. Use the 'uartRead()' function to determine if data is
 * available.
 *
 * @return the character read from the UART
 */
uint8_t uartRead();

/** Determine if data is available to read
 *
 * @return true if data is available and a call to 'uartRead()' would return
 *              immediately.
 */
bool uartAvail();

/** Initialise SPI interface
 *
 * Set up SPI0 as master in mode 0 (CPHA and CPOL = 0) at 10MHz with no delays.
 * SSEL is not assigned to an output pin, it must be controlled separately.
 */
void spiInit();

/** Transfer a single data byte via SPI
 *
 * @param data the 8 bit value to send
 *
 * @return the data received in the transfer
 */
uint8_t spiTransfer(uint8_t data);

/** Initialise the external hardware
 *
 * This function needs to be called after the SPI bus has been initialised and
 * the pin switch matrix has been set up. It will initialise the external SPI
 * components.
 */
void hwInit();

/** Read a page of data from the EEPROM
 *
 * Fill a buffer with data from the selected page. The buffer must be large
 * enough to hold the page (@see EEPROM_PAGE_SIZE).
 *
 * @param address the page address to read
 * @param pData pointer to the buffer to hold the data.
 */
void eepromReadPage(uint16_t address, uint8_t *pData);

/** Write a page of data to the EEPROM
 *
 * Write a buffer of data to the selected page. The buffer must be large
 * enough to hold the page (@see EEPROM_PAGE_SIZE).
 *
 * @param address the page address to write
 * @param pData pointer to the buffer holding the data.
 */
void eepromWritePage(uint16_t address, uint8_t *pData);

#ifdef __cplusplus
}
#endif

#endif /* __HARDWARE_H */

