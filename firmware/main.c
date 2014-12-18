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
  // Initialise the hardware
  hwInit();
  // And run the emulator
  cpuResetIO();
  cpuReset();
//  uint32_t start = getMillis();
//  uint32_t instructions = 0;
//  uint8_t qsec = 0;
  while (true) {
//    if(getDuration(start)>=250) {
//      // 1/4 second interval
//      g_ioState.m_kips = (uint16_t)(instructions / 250);
//      instructions = 0;
//      if(++qsec==4) {
//      	qsec = 0;
//      	g_ioState.m_time++;
//        }
//      }
    // Execute a single instruction
    cpuStep();
//    instructions++;
    }
  return 0;
  }

