/*--------------------------------------------------------------------------*
* Command definitions and lookup
*---------------------------------------------------------------------------*
* 22-Dec-2014 ShaneG
*
* Defines the text version of the commands (and aliases) and provides a
* method to identify the command.
*--------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "minmon.h"

/** Command text to ID mapping
 */
typedef struct _CMDINFO {
  const char      *m_cszCommand; //!< The text for the command or alias
  const MINMON_CMD m_command;    //!< The command ID
  } CMDINFO;

/** Command definitions
 *
 * This structure maps command text to a command ID. It is terminated by an
 * entry with the command ID set to CMD_INVALID.
 */
static CMDINFO s_commands[] = {
  { "A",     CMD_ADDRESS },
  { "ADDR",  CMD_ADDRESS },
  { "R",     CMD_READ    },
  { "READ",  CMD_READ    },
  { "W",     CMD_WRITE   },
  { "WRITE", CMD_WRITE   },
  { "G",     CMD_GOTO    },
  { "GOTO",  CMD_GOTO    },
  { "",      CMD_INVALID }
  };

/** Get the command ID from a string
 *
 * Convert a string into a command ID.
 *
 * @param cszCmd pointer to the string containing the command name.
 * @param length the length of the string in characters
 *
 * @return the command ID or CMD_INVALID if the string does not represent a
 *         valid command.
 */
MINMON_CMD cmdLookup(const char *cszCmd, uint8_t length) {
  uint8_t index = 0;
  while(s_commands[index].m_command!=CMD_INVALID) {
    if(strncmp(cszCmd, s_commands[index].m_cszCommand, length))
      return s_commands[index].m_command;
    index++;
    }
  return CMD_INVALID;
  }

