/*--------------------------------------------------------------------------*
* TGL-6502 Hardware Emulation
*---------------------------------------------------------------------------*
* 27-Nov-2014 ShaneG
*
* This provides the hardware interface to the SPI peripheral and provides
* the emulator access to them.
*--------------------------------------------------------------------------*/
#ifndef __TGL6502_H
#define __TGL6502_H

#ifdef __cplusplus
extern "C" {
#endif

//! Number of MMU pages (8 x 8K pages)
#define MMU_PAGE_COUNT 8

//! Size of the SPI transfer buffer
#define SPI_BUFFER_SIZE 128

//! The start of ROM space in physical memory
#define ROM_BASE 0x00080000L

//! The start of IO space in CPU memory
#define IO_BASE 0xFF00

#pragma pack(push, 1)

/** Represents the memory mapped IO area
 *
 * This structure maintains data for the memory mapped IO area. When modifying
 * the structure be sure that it does not exceed 250 bytes in size - the upper
 * 6 bytes are required for the 6502 interrupt vectors.
 */
typedef struct _IO_STATE {
  uint8_t     m_version;                 //!< RO: Version byte
  uint8_t     m_ioctl;                   //!< RW: IO control flags
  uint8_t     m_conin;                   //!< RO: Console input
  uint8_t     m_conout;                  //!< RW: Console output
  uint16_t    m_time;                    //!< RW: Current time (seconds since midnight)
  uint16_t    m_kips;                    //!< RO: Instructions per second (1000s)
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
  IO_OFFSET_VERSION = 0,
  IO_OFFSET_IOCTL = 1,
  IO_OFFSET_CONIN = 2,
  IO_OFFSET_CONOUT = 3,
  IO_OFFSET_TIME  = 4,
  IO_OFFSET_KIPS  = 6,
  IO_OFFSET_SLOT0 = 8,
  IO_OFFSET_SPICMD = 9,
  IO_OFFSET_SPIBUFF = 10,
  IO_OFFSET_PAGES = 10 + SPI_BUFFER_SIZE,
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

/** Types of interrupt that can be generated
 */
typedef enum {
  INT_IRQ, //!< The general interrupt request pin
  INT_NMI, //!< The non-maskable interrupt pin
  } INTERRUPT;

//---------------------------------------------------------------------------
// UART operations
//---------------------------------------------------------------------------

/** Send a single character over the UART
 *
 * This is a blocking operation. If the transmitter is not yet ready the
 * function will block until it can send the character.
 *
 * @param ch the character to send.
 */
void uartWrite(uint8_t ch);

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

//---------------------------------------------------------------------------
// Memory and simulated IO operations
//---------------------------------------------------------------------------

/** Initialise the memory and IO subsystem
 *
 * This function should be called every time the 'cpuReset()' function is
 * called. In ensures the memory mapped IO region is simulating the 'power on'
 * state.
 */
void cpuResetIO();

/** Read a single byte from the CPU address space.
*
* @param address the 16 bit address to read a value from.
*
* @return the byte read from the address.
*/
uint8_t cpuReadByte(uint16_t address);

/** Write a single byte to the CPU address space.
*
* @param address the 16 bit address to write a value to
*/
void cpuWriteByte(uint16_t address, uint8_t value);

//---------------------------------------------------------------------------
// Functions implemented by the emulator.
//---------------------------------------------------------------------------

/** Reset the CPU
 *
 * Simulates a hardware reset.
 */
void cpuReset();

/** Execute a single instruction
 */
void cpuStep();

/** Trigger an interrupt
 *
 * @param interrupt the type of interrupt to generate
 */
void cpuInterrupt(INTERRUPT interrupt);

//---------------------------------------------------------------------------
// Physical hardware initialisation and helpers
//---------------------------------------------------------------------------

/** Get the current timestamp in milliseconds
 *
 * This function is used for basic duration timing. The base of the timestamp
 * doesn't matter as long as the result increments every millisecond.
 *
 * @return timestamp in milliseconds
 */
uint32_t getMillis();

/** Get the duration between a previous timestamp and the current.
 *
 * Determine the duration between a previous timestamp and the current time.
 */
uint32_t getDuration(uint32_t since);

/** Initialise the external hardware
 *
 * This function is responsible for setting up the native hardware interface.
 */
void hwInit();

#ifdef __cplusplus
}
#endif

#endif /* __TGL6502_H */
