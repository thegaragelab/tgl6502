This is the source for the Extended Woz Monitor (eWoz). The original source can
be found here:

        http://www.brielcomputers.com/phpBB3/viewtopic.php?f=9&t=197

It is an extension of the original Woz monitor for the Apple with the following
extensions:

* All key strokes are converted to uppercase.
* The backspace works so the _ is no longer needed.
* When you run a program, it's called with an JSR so if the program ends with
  an RTS, you will be taken back to the monitor.
* You can load Intel HEX format files and it keeps track of the checksum.

