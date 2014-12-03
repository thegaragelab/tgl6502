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

/** Time of day information
 *
 * Provides a simple represention of the current time of day. There is no RTC
 * in the system so this starts at 00:00:00 after reset and increments every
 * second. This can be set by the emulated code to keep track of time.
 */
typedef struct _TIME_OF_DAY {
  uint8_t m_hour;   //!< Hour (24 hour format)
  uint8_t m_minute; //!< Minute
  uint8_t m_second; //!< Seconds
  } TIME_OF_DAY;

/** IO control bits
 */
typedef enum {
  IOCTL_CONDATA   = 0x01, //!< Data is available to read from the console
  IOCTL_PAGEWRITE = 0x02, //!< This bit must be set to enable writes to the page table
  } IOCTL_FLAGS;

/** Represents the memory mapped IO area
 *
 * This structure maintains data for the memory mapped IO area.
 */
typedef struct _IO_STATE {
  uint32_t    m_ips;                     //!< Instructions per second (read only)
  TIME_OF_DAY m_time;                    //!< Current time
  uint8_t     m_ioctl;                   //!< IO control flags
  uint8_t     m_conin;                   //!< Console input (read only)
  uint8_t     m_conout;                  //!< Console output (write only)
  uint8_t     m_slot0;                   //!< Slot 0 GPIO control
  uint8_t     m_slot1;                   //!< Slot 1 GPIO control
  uint8_t     m_spicmd;                  //!< SPI control byte
  uint8_t     m_spibuf[SPI_BUFFER_SIZE]; //!< SPI transfer buffer
  uint8_t     m_pages[MMU_PAGE_COUNT];   //!< MMU page map
  } IO_STATE;

#pragma pack(pop)

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

