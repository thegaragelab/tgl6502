/*---------------------------------------------------------------------------*
* Common interface to the TGL-6502 hardware
*----------------------------------------------------------------------------*
* 20-Dec-2014 ShaneG
*
* Provides data structures and constants for interacting with the TGL-6502
* memory mapped hardware. Also provides a handful of basic helper functions
* to manipulate the hardware.
*---------------------------------------------------------------------------*/
#ifndef __TGL6502_H
#define __TGL6502_H

//! Number of MMU pages (8 x 8K pages)
#define MMU_PAGE_COUNT 8

//! Size of the SPI transfer buffer
#define SPI_BUFFER_SIZE 128

//! The start of IO space in CPU memory
#define IO_BASE 0xFF00

/** The memory mapped IO structure
 *
 * The memory at IO_BASE is laid out according to this structure. Note that it
 * must match the structure defined in the emulator firmware.
 */
typedef struct _TGL6502_IO {
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
  } TGL6502_IO;

/** The access point for IO */
extern TGL6502_IO *pIO;

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

//---------------------------------------------------------------------------
// Basic console operations
//---------------------------------------------------------------------------

/** Get a single character from the console
 *
 * This function blocks until a character is ready and then returns the ASCII
 * code for that character. You can use the 'kbhit()' function to determine if
 * a character is ready to be read.
 *
 * @return the ASCII value of the character pressed.
 */
uint8_t getch();

/** Determine if a character is available to be read.
 *
 * This function can be used to implement non-blocking character IO without
 * interrupts. If it returns true the 'getch()' function will immediately
 * return a character without blocking.
 *
 * @return true if a character is available to be read.
 */
bool kbhit();

/** Write a single character to the console
 *
 * @param ch the character to write
 */
void putch(uint8_t ch);

/** Write a NUL terminated string to the console
 *
 * @param str the string to write.
 *
 * @return the number of characters written.
 */
int puts(const char *str);

#endif /* __TGL6502_H */

