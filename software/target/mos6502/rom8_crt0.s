; Startup code for TGL6502 8K ROM
;----------------------------------------------------------------------------

        .export         _exit
        .export         __STARTUP__ : absolute = 1      ; Mark as startup
        .import         callmain
        .import         initlib, donelib
        .import         exit
        .import         __RAM_START__, __RAM_SIZE__     ; Linker generated
        .import         __BSS_RUN__, __BSS_SIZE__


        .include        "zeropage.inc"

        .segment        "STARTUP"

        cld
        ldx     #$FF
        txs
        lda     #<(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)
        ldx     #>(__RAM_START__ + __RAM_SIZE__ + __STACKSIZE__)
        sta     sp
        stx     sp+1
        jsr     zerobss
        jsr     initlib
        jsr     callmain
_exit:  pha
        jsr     donelib
        pla
        jmp     exit

