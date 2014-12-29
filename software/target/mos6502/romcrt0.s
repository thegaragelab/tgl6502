; Startup code for TGL6502 ROM
;----------------------------------------------------------------------------

        .export         _exit
        .export         __STARTUP__ : absolute = 1      ; Mark as startup
        .import         callmain
        .import         initlib, donelib
        .import         zerobss, copydata
        .import         _intIRQ, _intNMI
        .import         __RAM_START__, __RAM_SIZE__     ; Linker generated
        .import         __STACKSIZE__                   ; Linker generated
        .import         __BSS_RUN__, __BSS_SIZE__


        .include        "zeropage.inc"

        .segment        "STARTUP"

init:   cld
        ldx     #$FF
        txs
        lda     #<(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)
        ldx     #>(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)
        sta     sp
        stx     sp+1
        jsr     zerobss
        jsr     copydata
        jsr     initlib
        jsr     callmain
_exit:  pha
        jsr     donelib
        pla
        ; This is for ROM, it should never exit
stop:   jmp     stop

        .segment        "VECTORS"

        .addr           _intNMI
        .addr           init
        .addr           _intIRQ

