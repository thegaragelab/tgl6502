; Default handler for NMI interrupts
;----------------------------------------------------------------------------
; 22-Dec-2014 ShaneG
;
; Provides the default NMI handler if none is provided by the main program.
;----------------------------------------------------------------------------

        .export _intNMI

        .segment "CODE"

_intNMI:
        rti     ; Just return

