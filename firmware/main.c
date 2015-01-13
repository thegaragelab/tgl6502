/*--------------------------------------------------------------------------*
* 6502 CPU Emulator - Test Harness
*---------------------------------------------------------------------------*
* 24-Nov-2014 ShaneG
*
* This is a simple Windows command line application to test the emulator.
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <tgl6502.h>

/** Program entry point
 */
int main(int argc, char *argv[]) {
  uint8_t ticks = 0;
  uint32_t instructions = 0;
  // Initialise the hardware
  hwInit();
  // And run the emulator
  cpuResetIO();
  cpuReset();
  while (true) {
    // Check for tick elapsed
    if(qsecElapsed()) {
      // Update instruction count
      instructions = instructions >> 6; // * 4 / 1024
      g_ioState.m_kips = (uint16_t)instructions;
      // Update time
      if(++ticks==4) {
        g_ioState.m_time++;
        ticks = 0;
        }
      // Trigger timer interrupt if needed
      if((g_ioState.m_ioctl&IOCTL_NMI_MASK)==IOCTL_NMI_TIMER)
        cpuInterrupt(INT_NMI);
      if((g_ioState.m_ioctl&IOCTL_IRQ_MASK)==IOCTL_IRQ_TIMER)
        cpuInterrupt(INT_IRQ);
      }
    // Execute a single instruction
    cpuStep();
    instructions++;
    }
  return 0;
  }

