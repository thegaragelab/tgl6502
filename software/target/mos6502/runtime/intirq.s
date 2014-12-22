; Default handler for IRQ interrupts
;----------------------------------------------------------------------------
; 22-Dec-2014 ShaneG
;
; Provides the default IRQ handler if none is provided by the main program.
;----------------------------------------------------------------------------

        .export _intIRQ

        .segment "CODE"

_intIRQ:
        rti     ; Just return

