;
;
;***************************************************
;* S/O/S SyMon III (c)1990,2007 By Brian M. Phelps *
;*                                                 *
;*         BIOS, Monitor, Micro-assembler          *
;*           For the 6502/65c02 family             *
;*         of microprocessor units (MPU)           *
;*                                                 *
;*   Includes text editor, text and byte string    *
;*  search utility, data upload/download utility   *
;*                                                 *
;*              Version 06/07/07                   *
;*                                                 *
;*          (Requires 8kb of memory)               *
;***************************************************
;
;14 byte buffer
INBUFF   .EQU $B0      ;14 bytes ($B0-$BD) 
;
;16-bit variables:
COMLO   .EQU $BE
COMHI   .EQU COMLO+1
DELLO   .EQU $C0
DELHI   .EQU DELLO+1
PROLO   .EQU $C2
PROHI   .EQU PROLO+1
BUFADR  .EQU $C4
BUFADRH .EQU BUFADR+1
TEMP2   .EQU $C6
TEMP2H  .EQU TEMP2+1
INDEX   .EQU $C8
INDEXH  .EQU INDEX+1
TEMP3   .EQU $CA
TEMP3H  .EQU TEMP3+1
;
;8-bit variables and constants:
TEMP     .EQU $CC
BUFIDX   .EQU $CD
BUFLEN   .EQU $CE
BSPIND   .EQU $CF
IDX      .EQU $D0
IDY      .EQU $D1
SCNT     .EQU $D2
OPLO     .EQU $D3
OPHI     .EQU $D4
STEMP    .EQU $D5
RT1      .EQU $D6
RT2      .EQU $D7
SETIM    .EQU $D8
LOKOUT   .EQU $D9
ACCUM    .EQU $DA
XREG     .EQU $DB
YREG     .EQU $DC
SREG     .EQU $DD
PREG     .EQU $DE
POINTER  .EQU $DF
HPHANTOM .EQU $00E0    ; HPHANTOM MUST be located (in target memory) immediatly below the HEX0AND1 variable
HEX0AND1 .EQU $E1
HEX2AND3 .EQU $E2
HEX4AND5 .EQU $E3
HEX6AND7 .EQU $E4
DPHANTOM .EQU $00E4    ; DPHANTOM MUST be located (in target memory) immediatly below the DEC0AND1 variable
DEC0AND1 .EQU $E5
DEC2AND3 .EQU $E6
DEC4AND5 .EQU $E7
DEC6AND7 .EQU $E8
DEC8AND9 .EQU $E9
INQTY    .EQU $EA
INCNT    .EQU $EB
OUTCNT   .EQU $EC
AINTSAV  .EQU $ED
XINTSAV  .EQU $EE
YINTSAV  .EQU $EF
;
;16 byte buffer
SRCHBUFF .EQU $00F0    ;16 bytes ($F0-$FF) (notice that this variable MUST be expressed as a 16 bit address
;                        even though it references a zero-page address)   
;
;Keystroke input buffer address:
KEYBUFF  .EQU $0200    ;256 bytes: ($200-$2FF) keystrokes (data from ACIA) are stored here
                       ; by SyMon's IRQ service routine.  
;
;ACIA device address:
SIODAT   .EQU $8000    ;ACIA data register   <--put your 6551 ACIA base address here (REQUIRED!)************************
SIOSTAT  .EQU SIODAT+1 ;ACIA status REGISTER
SIOCOM   .EQU SIODAT+2 ;ACIA command REGISTER
SIOCON   .EQU SIODAT+3 ;ACIA control REGISTER
;
;LCD module address:
LCDCOM   .EQU $9000    ;LCD command register <--Put your LCD module base address here (optional, see: COLDSTRT)*********
LCDDATA  .EQU LCDCOM+1
;
         .ORG $E000    ;Target address range $E000 through $FFFF will be used
;
;
;***************
;* Subroutines *
;***************
;
;ASC2BN subroutine: Convert 2 ASCII HEX digits to a binary (byte) value.
; Enter: ACCUMULATOR = high digit, Y-REGISTER = low digit
; Returns: ACCUMULATOR = binary value 
ASC2BN   PHA           ;Save high digit on STACK
         TYA           ;Copy low digit to ACCUMULATOR
         JSR  BINARY   ;Convert digit to binary nybble: result in low nybble
         STA  TEMP     ;Store low nybble
         PLA           ;Pull high digit from STACK
         JSR  BINARY   ;Convert digit to binary nybble: result in low nybble
         ASL  A        ;Move low nybble to high nybble 
         ASL  A
         ASL  A
         ASL  A
         ORA  TEMP     ;OR high nybble with stored low nybble 
         RTS           ;Done ASC2BN subroutine, RETURN
BINARY   SEC           ;Clear BORROW
         SBC  #$30     ;Subtract $30 from ASCII HEX digit
         CMP  #$0A     ;GOTO BNOK IF result < $0A     
         BCC  BNOK
         SBC  #$07     ; ELSE, subtract $07 from result
BNOK     RTS           ;Done BINARY subroutine, RETURN
;
;ASCTODEC subroutine: convert ASCII DECIMAL digits to BCD
ASCTODEC LDA  #$00     ;Initialize (zero) BCD digit input buffer:
         STA  DEC0AND1 ; two most significant BCD digits
         STA  DEC2AND3
         STA  DEC4AND5
         STA  DEC6AND7
         STA  DEC8AND9 ; two least significant BCD digits
         LDX  BUFIDX   ;Read number of digits entered: ASCII digit buffer index
         BEQ  A2DDONE  ;GOTO A2DDONE IF BUFIDX = 0: no digits were entered
         LDY  #$05     ; ELSE, Initialize BCD input buffer index: process up to 5 BCD bytes (10 digits)
ATODLOOP JSR  A2DSUB   ;Read ASCII digit then convert to BCD 
         STA  DPHANTOM,Y ;Write BCD digit to indexed buffer location (index always > 0)
         JSR  A2DSUB   ;Read ASCII digit then convert to BCD 
         ASL  A        ;Make this BCD digit the more significant in the BCD byte 
         ASL  A
         ASL  A
         ASL  A         
         ORA  DPHANTOM,Y ;OR with the less significant digit  
         STA  DPHANTOM,Y ;Write BCD byte to indexed buffer location (index always > 0)
         DEY           ;Decrement BCD input buffer index 
         BNE  ATODLOOP ;GOTO ATODLOOP IF buffer index <> 0: there is room to process another digit
A2DDONE  RTS           ; ELSE, done ASCTODEC, RETURN
;Read indexed ASCII DECIMAL digit from text buffer then convert digit to 4 bit BCD
A2DSUB   TXA           ;GOTO A2DCONV IF digit buffer index <> 0: there are more digits to process
         BNE  A2DCONV
         PLA           ; ELSE, pull return address from STACK
         PLA
         RTS           ;Done ASCTODEC, RETURN
A2DCONV  LDA  INBUFF-1,X ;Read indexed ASCII DECIMAL digit 
         SEC           ;Subtract ASCII "0" from ASCII DECIMAL digit: convert digit to BCD
         SBC  #$30
         DEX           ;Decrement ASCII digit buffer index
         RTS           ;A2DSUB done, RETURN
;
;BCDOUT subroutine: convert 10 BCD digits to ASCII DECIMAL digits then send result to terminal. 
;Leading zeros are supressed in the displayed result.
;Call with 10 digit (5 byte) BCD value contained in variables DEC0AND1 through DEC8AND9:
;DEC0AND1 ($E5) Two most significant BCD digits
;DEC2AND3 ($E6)
;DEC4AND5 ($E7)
;DEC6AND7 ($E8)
;DEC8AND9 ($E9) Two least significant BCD digits
BCDOUT   LDX  #$00     ;Initialize BCD output buffer index: point to MSB
         LDY  #$00     ;Initialize leading zero flag: no non-zero digits have been processed
BCDOUTL  LDA  DEC0AND1,X ;Read indexed byte from BCD output buffer
         LSR  A        ;Shift high digit to low digit, zero high digit
         LSR  A
         LSR  A
         LSR  A
         JSR  BCDTOASC ;Convert BCD digit to ASCII DECIMAL digit, send digit to terminal
         LDA  DEC0AND1,X ;Read indexed byte from BCD output buffer
         AND  #$0F     ;Zero the high digit
         JSR  BCDTOASC ;Convert BCD digit to ASCII DECIMAL digit, send digit to terminal
         INX           ;Increment BCD output buffer index
         CPX  #$05
         BNE  BCDOUTL  ;LOOP back to BCDOUTL IF output buffer index <> 5
         CPY  #$00
         BNE  BCDOUTDN ; ELSE, GOTO BCDOUTDN IF any non-zero digits were processed
         LDA  #$30     ; ELSE, send "0" to terminal
         JSR  COUT
BCDOUTDN RTS           ;Done BCDOUT subroutine, RETURN
;BCDTOASC subroutine: 
; convert BCD digit to ASCII DECIMAL digit, send digit to terminal IF it's not a leading zero
BCDTOASC BNE  NONZERO  ;GOTO NONZERO IF BCD digit <> 0
         CPY  #$00     ; ELSE, GOTO BTADONE IF no non-zero digits have been processed 
         BEQ  BTADONE  ;  (supress output of leading zeros)
NONZERO  INY           ; ELSE, indicate that a non-zero digit has been processed (Y REGISTER <> 0) 
         CLC           ;Add ASCII "0" to digit: convert BCD digit to ASCII DECIMAL digit
         ADC  #$30
         JSR  COUT     ;Send converted digit to terminal
BTADONE  RTS           ;Done BCDTOASC subroutine, RETURN
;
;BCDTOHEX subroutine: convert a 1-10 digit BCD value to a 1-8 digit HEX value.
; Call with 10 digit (5 byte) DECIMAL value in DEC0AND1(MSB) through DEC8AND9(LSB).
; Returns with 8 digit (4 byte) HEX result in HEX0AND1(MSB) through HEX6AND7(LSB)
;DPHANTOM is a 16 bit address used to reference an 8 bit zero-page address.
; (BCDTOHEX needs LDA $hh,Y (an invalid instruction) so we use LDA $00hh,Y instead)
; This address is not written-to nor read-from in the BCDTOHEX subroutine.
; The address is the zero-page memory location immediatly below the DEC0AND1 variable
;BCD value input buffer:
;DEC0AND1 ;Two most significant BCD digits
;DEC2AND3
;DEC4AND5
;DEC6AND7
;DEC8AND9 ;Two least significant BCD digits
;HEX value output buffer (HEX accumulator):
;HEX0AND1 Two most significant HEX digits
;HEX2AND3
;HEX4AND5
;HEX6AND7 Two least significant HEX digits
BCDTOHEX LDA  #$00     ;Initialize (zero) output buffer. This is an 8 digit (4 byte) HEX accumulator
         STA  HEX0AND1
         STA  HEX2AND3
         STA  HEX4AND5
         STA  HEX6AND7
         LDY  #$05     ;Initialize DECIMAL input buffer byte index: point to (address - 1) of LSB
         LDX  #$03     ;Initialize multiplicand table index: point to LSB of lowest multiplicand
BCDLOOP  LDA  DPHANTOM,Y ;Read indexed byte from input buffer: Y REGISTER index always > 0 here
         AND  #$0F     ;Zero the high digit
         JSR  MULTPLI  ;Multiply low digit
         INX           ;Add 4 to multiplicand table index: point to LSB of next higher multiplicand
         INX
         INX
         INX
         LDA  DPHANTOM,Y ;Read indexed byte from input buffer: Y REGISTER index always > 0 here 
         LSR  A        ;Shift high digit to low digit, zero high digit
         LSR  A
         LSR  A
         LSR  A
         JSR  MULTPLI  ;Multiply digit
         INX           ;Add 4 to multiplicand table index: point to LSB of next higher multiplicand
         INX
         INX
         INX
         DEY           ;Decrement DECIMAL input buffer byte index
         BNE  BCDLOOP  ;LOOP back to BCDLOOP IF byte index <> 0: there are more bytes to process
         RTS           ; ELSE, done BCDTOHEX subroutine, RETURN
;Multiply indexed multiplicand by digit in ACCUMULATOR
MULTPLI  JSR  SAVREGS  ;Save A,X,Y REGISTERS on STACK
         TAY           ;Copy digit to Y REGISTER: multiplier loop counter
DMLTLOOP CPY  #$00
         BNE  DDOADD   ;GOTO DDOADD IF multiplier loop counter <> 0 
         JSR  RESREGS  ; ELSE, pull A,X,Y REGISTERS from STACK
         RTS           ;Done MULTIPLI subroutine, RETURN
;Add indexed multiplicand to HEX accumulator (output buffer)
DDOADD   CLC
         LDA  DMULTAB,X ;Least significant byte of indexed multiplicand
         ADC  HEX6AND7  ;Least significant byte of HEX accumulator
         STA  HEX6AND7
         LDA  DMULTAB-1,X
         ADC  HEX4AND5
         STA  HEX4AND5
         LDA  DMULTAB-2,X
         ADC  HEX2AND3
         STA  HEX2AND3
         LDA  DMULTAB-3,X ;Most significant byte of indexed multiplicand
         ADC  HEX0AND1 ;Most significant byte of HEX accumulator
         STA  HEX0AND1
         DEY           ;Decrement multiplier loop counter
         BCS  OVERFLOW ;GOTO OVERFLOW IF the last add produced a CARRY: HEX output buffer has overflowed 
         BCC  DMLTLOOP ; ELSE, LOOP back to DMLTLOOP (always branch)
OVERFLOW LDA  #$2A     ;Send "*" to terminal: indicate that an overflow has occured
         JSR  COUT
         JMP  DMLTLOOP ;LOOP back to DMLTLOOP  
;HEX multiplicand table:
DMULTAB  .DB  $00, $00, $00, $01 ;HEX weight of least significant BCD digit
         .DB  $00, $00, $00, $0A
         .DB  $00, $00, $00, $64
         .DB  $00, $00, $03, $E8
         .DB  $00, $00, $27, $10
         .DB  $00, $01, $86, $A0
         .DB  $00, $0F, $42, $40
         .DB  $00, $98, $96, $80
         .DB  $05, $F5, $E1, $00
         .DB  $3B, $9A, $CA, $00 ;HEX weight of most significant BCD digit
;
;BEEP subroutine: Send ASCII [BELL] to terminal
BEEP     PHA           ;Save ACCUMULATOR on STACK
         LDA  #$07     ;Send ASCII [BELL] to terminal
         JSR  COUT
         PLA           ;Restore ACCUMULATOR from STACK
         RTS           ;Done BEEP subroutine, RETURN
;
;BN2ASC subroutine: Convert byte in ACCUMULATOR to two ASCII HEX digits.
; Returns: ACCUMULATOR = high digit, Y-REGISTER = low digit
BN2ASC   TAX           ;Copy ACCUMULATOR to X-REGISTER
         AND  #$F0     ;Mask (zero) low nybble
         LSR  A        ;Move high nybble to low nybble: high nybble now = 0
         LSR  A
         LSR  A
         LSR  A
         JSR  ASCII    ;Convert nybble to an ASCII HEX digit
         PHA           ;Save high digit on STACK
         TXA           ;Restore ACCUMULATOR from X-REGISTER
         AND  #$0F     ;Mask (zero) high nybble
         JSR  ASCII    ;Convert nybble to an ASCII HEX digit
         TAY           ;Copy low digit to Y-REGISTER
         PLA           ;Pull high digit from STACK to ACCUMULATOR
         RTS           ;Done BN2ASC subroutine, RETURN
ASCII    CMP  #$0A     ;GOTO ASOK IF nybble < $A
         BCC  ASOK
         CLC           ; ELSE, clear CARRY
         ADC  #$07     ;ADD $07
ASOK     ADC  #$30     ;ADD $30
         RTS           ;Done BN2ASC or ASCII subroutine, RETURN
;
;BSOUT subroutine: send [BACKSPACE] to terminal
BSOUT    LDA  #$08     ;Send [BACKSPACE] to terminal
         JMP  COUT     ; then done BSOUT subroutine, RETURN
;
;CHIN subroutine: Wait for a keystroke, return with keystroke in ACCUMULATOR
CHIN     STX  TEMP     ;Save X REGISTER
CHINLOOP CLI           ;Enable external IRQ response
         LDA  INCNT    ;(Keystroke buffer input counter)
         CMP  OUTCNT   ;(Keystroke buffer output counter)
         BEQ  CHIN     ;LOOP back to CHIN IF INCNT = OUTCNT: there are no keystrokes in buffer 
         LDX  OUTCNT   ; ELSE, read indexed keystroke from buffer
         LDA  KEYBUFF,X
         INC  OUTCNT   ;Remove keystroke from buffer
         LDX  TEMP     ;Restore X REGISTER
         RTS           ;Done CHIN subroutine, RETURN
;
;CHREG subroutine: Display HEX byte value in ACCUMULATOR, request HEX byte input from terminal
CHREG    JSR  PRBYTE   ;Display HEX value of ACCUMULATOR
         JSR  SPC      ;Send [SPACE] to terminal
         JMP  HEXIN2   ;Request HEX byte input from terminal (result in ACCUMULATOR)
                       ; then done CHREG subroutine, RETURN
;
;COUT subroutines: Send byte in ACCUMULATOR to terminal (1,2 or 3 times)
COUT3    JSR  COUT     ;(Send byte 3 times)
COUT2    JSR  COUT     ;(Send byte 2 times)
COUT     PHA           ;Save ACCUMULATOR on STACK
COUTL    LDA  SIOSTAT  ;Read ACIA status register
         AND  #$10     ;Isolate transmit data register status bit
         BEQ  COUTL    ;LOOP back to COUTL IF transmit data register is full
         PLA           ; ELSE, restore ACCUMULATOR from STACK
         STA  SIODAT   ;Write byte to ACIA transmit data register
         RTS           ;Done COUT subroutine, RETURN
;
;DECINPUT subroutine: request 1 to 10 DECIMAL digits from terminal. Result is
; stored in zero-page address INBUFF through (INBUFF + $0A)  
;Setup RDLINE subroutine parameters:
DECINPUT LDA  #$FF     ;  Allow only valid ASCII DECIMAL digits to be entered:
         STA  LOKOUT   ;   variable LOKOUT = $FF
         LDY  #INBUFF  ;  Y-REGISTER = buffer address low byte
         LDA  #$00     ;  ACCUMULATOR = buffer address high byte
         LDX  #$0A     ;  X-REGISTER = maximum number of digits allowed
         JSR  RDLINE   ;Request ASCII DECIMAL digit(s) input from terminal
         RTS           ;Done DECINPUT subroutine, RETURN
;
;DECINDEX subroutine: decrement 16 bit system variable INDEX,INDEXH
DECINDEX SEC
         LDA  INDEX
         SBC  #$01
         STA  INDEX
         LDA  INDEXH
         SBC  #$00
         STA  INDEXH
         RTS           ;Done DECINDEX subroutine, RETURN
;
;DOLLAR subroutine: Send "$" to terminal
;CROUT subroutines: Send CR,LF to terminal (1 or 2 times)
DOLLAR   PHA           ;Save ACCUMULATOR on STACK
         LDA  #$24     ;Send "$" to terminal
         BNE  SENDIT   ;GOTO SENDIT
;Send CR,LF to terminal 2 times
CR2      JSR  CROUT    ;Send CR,LF to terminal
;Send CR,LF to terminal 1 time     
CROUT    PHA           ;Save ACCUMULATOR
         LDA  #$0D     ;Send [RETURN] to terminal
         JSR  COUT
         LDA  #$0A     ;Send [LINEFEED] to terminal
SENDIT   JSR  COUT
         PLA           ;Restore ACCUMULATOR from STACK
         RTS           ;Done CROUT or DOLLAR subroutine, RETURN
;
;DELAY1 subroutine: short delay loop, duratiion is (1 * DELLO)
;DELAY2 subroutine: medium delay loop, duratiion is (DELHI * DELLO)
DELAY1   LDX  #$01     ;Preset delay multiplier to 1
         JMP  ITER
DELAY2   LDX  DELHI    ;Read delay multiplier variable
ITER     LDY  DELLO    ;Read delay duration variable
WAIT     DEY           ;Decrement duration counter
         BNE  WAIT     ;LOOP back to WAIT IF duration <> 0
         DEX           ; ELSE, decrement multiplier
         BNE  ITER     ;LOOP back to ITER IF multiplier <> 0
         RTS           ; ELSE, done DELAYn subroutine, RETURN
;
;GLINE subroutine: Send a horizontal line to terminal 
GLINE    LDX  #$4F
         LDA  #$7E     ;Send "~" to terminal 79 times
GLINEL   JSR  COUT
         DEX
         BNE  GLINEL
         RTS           ;Done GLINE subroutine, RETURN
;
;HEXIN subroutines: Request 1 to 4 ASCII HEX digit input from terminal
; then convert digits into a binary (byte or word) value
;IF 1 OR 2 digits are REQUESTED, returns byte in ACCUMULATOR
;IF 3 OR 4 digits are REQUESTED, returns word in variable INDEX (low byte),INDEXH (high byte)
;Variable SCNT will contain the number of digits entered
HEXIN2   LDA  #$02     ;Request 2 ASCII HEX digits from terminal: 8 bit value 
         BNE  HEXIN    ;GOTO HEXIN: always branch
HEXIN4   LDA  #$04     ;Request 4 ASCII HEX digits from terminal: 16 bit value
HEXIN    STA  INQTY    ;Store number of digits to allow: value = 1 to 4 only
         JSR  DOLLAR   ;Send "$" to terminal
;Setup RDLINE subroutine parameters:
         LDA  #$80     ;  Allow only valid ASCII HEX digits to be entered:
         STA  LOKOUT   ;   make variable LOKOUT <> 0
         LDA  #$00     ;  ACCUMULATOR = buffer address high byte 
         LDY  #INBUFF  ;  Y-REGISTER = buffer address low byte
         LDX  INQTY    ;  X-REGISTER = maximum number of digits allowed
         JSR  RDLINE   ;Request ASCII HEX digit(s) input from terminal
         STX  SCNT     ;Save number of digits entered to SCNT
         LDA  INQTY    ;GOTO JUST4 IF 4 digits were REQUESTED
         CMP  #$04
         BEQ  JUST4
         CMP  #$03     ; ELSE, GOTO JUST3 IF 3 digits were REQUESTED
         BEQ  JUST3
         CPX  #$00     ; ELSE, GOTO IN2 IF 0 OR 2 digits were entered
         BEQ  IN2
         CPX  #$02
         BEQ  IN2
         JSR  JUSTBYTE ; ELSE, move digit from INBUFF to INBUFF+1, write "0" to INBUFF
IN2      LDA  INBUFF   ;Convert 2 ASCII HEX digits in INBUFF(high digit),
         LDY  INBUFF+1 ; INBUFF+1(low digit) to a binary value,
         JMP  ASC2BN   ; (result in ACCUMULATOR) then RETURN (done HEXIN subroutine) 
JUST3    CPX  #$00     ;GOTO IN3 IF 0 OR 3 digits were entered
         BEQ  IN3
         CPX  #$03
         BEQ  IN3
         CPX  #$02     ; ELSE, GOTO SHFT3 IF 2 digits were entered
         BEQ  SHFT3
         JSR  JUSTBYTE ; ELSE, move digit from INBUFF to INBUFF+1, write "0" to INBUFF
SHFT3    JSR  SHIFT    ;Move digits from INBUFF+1 to INBUFF+2, INBUFF to INBUFF+1
IN3      LDA  #$30     ;Convert 2 ASCII HEX digits, "0"(high digit),
         LDY  INBUFF   ; INBUFF(low digit) to a binary value,
         JSR  ASC2BN   ; result in ACCUMULATOR
         STA  INDEXH   ;Store in variable INDEXH(high byte)
         LDA  INBUFF+1 ;Convert 2 ASCII HEX digits in INBUFF+1(high digit),
         LDY  INBUFF+2 ; INBUFF+2(low digit) to a binary value,
         JSR  ASC2BN   ; result in ACCUMULATOR
         STA  INDEX    ;Store in variable INDEX(low byte)
         RTS           ;Done HEXIN subroutine, RETURN
JUST4    CPX  #$00     ;GOTO IN4 IF 0 OR 4 digits were entered
         BEQ  IN4
         CPX  #$04
         BEQ  IN4
         CPX  #$03     ; ELSE, GOTO X3 IF 3 digits were entered
         BEQ  X3
         CPX  #$02     ; ELSE, GOTO X2 IF 2 digits were entered
         BEQ  X2
         JSR  JUSTBYTE ; ELSE, move digit from INBUFF to INBUFF+1, write "0" to INBUFF
X2       JSR  SHIFT    ;Move digits from INBUFF+1 to INBUFF+2, INBUFF to INBUFF+1
X3       LDA  INBUFF+2 ;Move digits from INBUFF+2 to INBUFF+3
         STA  INBUFF+3
         JSR  SHIFT    ;Move digits from INBUFF+1 to INBUFF+2, INBUFF to INBUFF+1
IN4      LDA  INBUFF   ;Convert 2 ASCII HEX digits in INBUFF(high digit),
         LDY  INBUFF+1 ; INBUFF+1(low digit) to a binary value,
         JSR  ASC2BN   ; result in ACCUMULATOR
         STA  INDEXH   ;Store in variable INDEXH(high byte)
         LDA  INBUFF+2 ;Convert 2 ASCII HEX digits in INBUFF+2(high digit),
         LDY  INBUFF+3 ; INBUFF+3(low digit) to a binary value,
         JSR  ASC2BN   ; result in ACCUMULATOR
         STA  INDEX    ;Store in variable INDEX(low byte)
         RTS           ;Done HEXIN subroutine, RETURN
SHIFT    LDA  INBUFF+1 ;Move digit from INBUFF+1 to INBUFF+2
         STA  INBUFF+2
JUSTBYTE LDA  INBUFF   ;Move digit from INBUFF to INBUFF+1
         STA  INBUFF+1
         LDA  #$30     ;Write "0" to INBUFF
         STA  INBUFF
         RTS           ;Done SHIFT or JUSTBYTE subroutine, RETURN
;
;HEXINPUT subroutine: request 1 to 8 digit HEX value input from terminal then convert ASCII HEX to HEX
;Setup RDLINE subroutine parameters:
HEXINPUT LDA  #$80     ;  Allow only valid ASCII HEX digits to be entered:
         STA  LOKOUT   ;   variable LOKOUT <> 0
         LDX  #$08     ;  X-REGISTER = maximum number of digits allowed
         LDA  #$00     ;  ACCUMULATOR = buffer address high byte
         LDY  #INBUFF  ;  Y-REGISTER = buffer address low byte
         JSR  RDLINE   ;Request ASCII HEX digit(s) input from terminal
;Convert ASCII HEX digits to HEX
ASCTOHEX LDA  #$00     ;Initialize (zero) HEX digit input buffer:
         STA  HEX0AND1 ; Two most significant HEX digits
         STA  HEX2AND3
         STA  HEX4AND5
         STA  HEX6AND7 ; Two least significant HEX digits
         LDY  #$04     ;Initialize HEX input buffer index: process 4 bytes
ASCLOOP  STY  SCNT     ;Save HEX input buffer index
         LDA  $INBUFF-1,X ;Read indexed ASCII HEX digit from RDLINE buffer 
         TAY           ;Copy digit to Y REGISTER: least significant digit
         LDA  #$30     ;Make ACCUMULATOR = ASCII "0": MS digit 
         JSR  ASC2BN   ;Convert ASCII digits to binary value
         LDY  SCNT     ;Read saved HEX input buffer index
         STA  HPHANTOM,Y ;Write byte to indexed HEX input buffer location 
         DEX           ;Decrement ASCII digit count
         BEQ  HINDONE  ;GOTO HINDONE IF no ASCII digits are left to process
         LDY  #$30     ; ELSE, make Y REGISTER = ASCII "0": LS digit
         LDA  $INBUFF-1,X ;Read indexed ASCII HEX digit from RDLINE buffer: MS digit
         JSR  ASC2BN   ;Convert ASCII digits to binary value
         LDY  SCNT     ;Read saved HEX input buffer index
         ORA  HPHANTOM,Y ;OR high digit with low digit
         STA  HPHANTOM,Y ;Write byte to indexed HEX input buffer location 
         DEX           ;Decrement ASCII digit count
         BEQ  HINDONE  ;GOTO HINDONE IF no ASCII digits left to process
         DEY           ; ELSE, decrement HEX input buffer index
         BNE  ASCLOOP  ;LOOP back to ASCLOOP IF HEX input buffer is full  
HINDONE  RTS           ; ELSE, Done HEXINPUT subroutine, RETURN
;
;HEXOUT subroutine: convert 8 HEX digits to ASCII HEX digits then send result to terminal. 
;Leading zeros are supressed in the displayed result.
;Call with 8 digit (4 byte) HEX value contained in variables HEX0AND1 through HEX6AND7:
;HEX0AND1 Two most significant HEX digits
;HEX2AND3
;HEX4AND5
;HEX6AND7 Two least significant HEX digits
HEXOUT   LDX  #$00     ;Initialize HEX output buffer index: point to MSB
         LDY  #$00     ;Initialize leading zero flag: no non-zero digits have been processed
HEXOUTL  LDA  $HEX0AND1,X ;Read indexed byte from HEX output buffer
         LSR  A        ;Shift high digit to low digit, zero high digit
         LSR  A
         LSR  A
         LSR  A
         JSR  HEXTOASC ;Convert HEX digit to ASCII HEX digit, send digit to terminal
         LDA  $HEX0AND1,X ;Read indexed byte from HEX output buffer
         AND  #$0F     ;Zero the high digit
         JSR  HEXTOASC ;Convert HEX digit to ASCII HEX digit, send digit to terminal
         INX           ;Increment HEX output buffer index
         CPX  #$04
         BNE  HEXOUTL  ;LOOP back to HEXOUTL IF output buffer index <> 4 
         CPY  #$00
         BNE  HEXOUTDN ; ELSE, GOTO HEXOUTDN IF any non-zero digits were processed
         LDA  #$30     ; ELSE, send "0" to terminal
         JSR  COUT
HEXOUTDN RTS           ;Done HEXOUT subroutine, RETURN
;
;HEXTOASC subroutine: 
; convert HEX digit to ASCII HEX digit then send digit to terminal IF it's not a leading zero
HEXTOASC BNE  HNONZERO ;GOTO HNONZERO IF HEX digit <> 0
         CPY  #$00     ; ELSE, GOTO HTADONE IF no non-zero digits have been processed 
         BEQ  HTADONE  ;  (supress output of leading zeros)
HNONZERO INY           ; ELSE, indicate that a non-zero digit has been processed (Y REGISTER <> 0) 
         PHA           ;Save HEX digit on STACK
         SEC
         SBC  #$0A
         BCS  HEXDIGIT ;GOTO HEXDIGIT IF digit > 9
         PLA           ; ELSE, restore digit from STACK
         ADC  #$30     ;Add ASCII "0" to digit: convert $00 through $09 to ASCII "0" through "9"
         BCC  SENDIGIT ;GOTO SENDIGIT: always branch
HEXDIGIT CLC
         PLA           ;Restore digit from STACK
         ADC  #$37     ;Add ASCII "7" to digit: convert $0A through $0F to ASCII "A" through "F"
SENDIGIT JSR  COUT     ;Send converted digit to terminal
HTADONE  RTS           ;Done HEXTOASC subroutine, RETURN
;
;HEXTOBCD subroutine: convert a 1-8 digit HEX value to a 1-10 digit BCD value.
; Call with 8 digit (4 byte) HEX value in HEX0AND1(MSB) through HEX6AND7(LSB).
; Returns with 10 digit (5 byte) BCD result in DEC0AND1(MSB) through DEC8AND9(LSB)
;HPHANTOM is a 16 bit address used to reference an 8 bit zero-page address.
; (HEXTOBCD needs LDA $hh,Y (an invalid instruction) so we use LDA $00hh,Y instead)
; This address is not written-to nor read-from in the HEXTOBCD subroutine.
; The address is the zero-page memory location immediatly below the HEX0AND1 variable
;HEX value input buffer:
;HEX0AND1 Two most significant HEX digits
;HEX2AND3
;HEX4AND5
;HEX6AND7 Two least significant HEX digits
;BCD value output buffer (BCD accumulator):
;DEC0AND1 ;Two most significant BCD digits
;DEC2AND3
;DEC4AND5
;DEC6AND7
;DEC8AND9 ;Two least significant BCD digits
HEXTOBCD LDA  #$00     ;Initialize (zero) output buffer. This is a 10 digit (5 byte) BCD accumulator
         STA  DEC0AND1
         STA  DEC2AND3
         STA  DEC4AND5
         STA  DEC6AND7
         STA  DEC8AND9
         LDY  #$04     ;Initialize HEX input buffer byte index: point to address minus 1 of LSB
         LDX  #$04     ;Initialize multiplicand table index: point to LSB of lowest multiplicand
DECLOOP  LDA  HPHANTOM,Y ;Read indexed byte from input buffer: Y REGISTER index always > 0 here 
         AND  #$0F     ;Zero the high digit
         JSR  MULTIPLY ;Multiply low digit
         INX           ;Add 5 to multiplicand table index: point to LSB of next higher multiplicand
         INX
         INX
         INX
         INX
         LDA  HPHANTOM,Y ;Read indexed byte from input buffer: Y REGISTER index always > 0 here
         LSR  A        ;Shift high digit to low digit, zero high digit
         LSR  A
         LSR  A
         LSR  A
         JSR  MULTIPLY ;Multiply digit
         INX           ;Add 5 to multiplicand table index: point to LSB of next higher multiplicand
         INX
         INX
         INX
         INX
         DEY           ;Decrement HEX input buffer byte index 
         BNE  DECLOOP  ;LOOP back to DECLOOP IF byte index <> 0: there are more bytes to process
         RTS           ; ELSE, done HEXTOBCD subroutine, RETURN
;Multiply indexed multiplicand by digit in ACCUMULATOR
MULTIPLY JSR  SAVREGS  ;Save A,X,Y REGISTERS on STACK
         SED           ;Switch processor to BCD arithmatic mode
         TAY           ;Copy digit to Y REGISTER: multiplier loop counter 
HMLTLOOP CPY  #$00
         BNE  HDOADD   ;GOTO HDOADD IF multiplier loop counter <> 0
         CLD           ; ELSE, switch processor to BINARY arithmatic mode
         JSR  RESREGS  ;Pull A,X,Y REGISTERS from STACK
         RTS           ;Done MULTIPLY subroutine, RETURN
;Add indexed multiplicand to BCD accumulator (output buffer)
HDOADD   CLC
         LDA  HMULTAB,X ;Least significant byte of indexed multiplicand
         ADC  DEC8AND9 ;Least significant byte of BCD accumulator
         STA  DEC8AND9
         LDA  HMULTAB-1,X
         ADC  DEC6AND7
         STA  DEC6AND7
         LDA  HMULTAB-2,X
         ADC  DEC4AND5
         STA  DEC4AND5
         LDA  HMULTAB-3,X
         ADC  DEC2AND3
         STA  DEC2AND3
         LDA  HMULTAB-4,X ;Most significant byte of indexed multiplicand
         ADC  DEC0AND1 ;Most significant byte of BCD accumulator
         STA  DEC0AND1
         DEY           ;Decrement multiplier loop counter
         JMP  HMLTLOOP ;LOOP back to HMLTLOOP
;BCD multiplicand table:
HMULTAB  .DB $00, $00, $00, $00, $01 ;BCD weight of least significant HEX digit  
         .DB $00, $00, $00, $00, $16
         .DB $00, $00, $00, $02, $56
         .DB $00, $00, $00, $40, $96
         .DB $00, $00, $06, $55, $36
         .DB $00, $01, $04, $85, $76
         .DB $00, $16, $77, $72, $16
         .DB $02, $68, $43, $54, $56 ;BCD weight of most significant HEX digit
;
;INCINDEX subroutine: increment 16 bit system variable INDEX,INDEXH
INCINDEX CLC
         LDA  INDEX
         ADC  #$01
         STA  INDEX
         LDA  INDEXH
         ADC  #$00
         STA  INDEXH
         RTS           ;Done INCINDEX subroutine, RETURN
;
;MEMDMP subroutine: Send a formatted ASCII or HEX memory dump to terminal 
MEMDMP   STA  TEMP3    ;Store dump command type: 0 = HEX dump, non-0 = ASCII dump  
         JSR  DMPGR    ;Send address offsets to terminal 
         JSR  CROUT    ;Send CR,LF to terminal
         JSR  GLINE    ;Send horizontal line to terminal
         JSR  CROUT    ;Send CR,LF to terminal
         LDA  SCNT     ;GOTO NEWADR IF an address was entered: SCNT <> 0
         BNE  NEWADR
         LDA  TEMP2    ; ELSE, point to next consecutive memory page
         STA  INDEX    ;  address saved during last memory dump
         LDA  TEMP2H
         STA  INDEXH
         JMP  DLINE
NEWADR   JSR  IN4      ;Convert keystrokes in INBUFF thru INBUFF + 3 to binary, result in INDEX,INDEXH
DLINE    JSR  SPC4     ;Send 4 [SPACE] to terminal
         JSR  DOLLAR   ;Send "$" to terminal
         JSR  PRINDX   ;Send line base address to terminal
         JSR  SPC4     ;Send 4 [SPACE] to terminal
         LDY  #$00     ;Initialize line byte counter
         STY  IDY
GETBYT   LDA  (INDEX),Y ;Read indexed byte 
         LDX  TEMP3    ;GOTO DUMPH IF TEMP3 = 0: HEX value output was requested 
         BEQ  DUMPH
         JSR  PRASC    ; ELSE, Display byte as a printable ASCII character, send "." IF not printable
         JSR  SPC      ;Send [SPACE] to terminal
         JMP  DUMPA    ;GOTO DUMPA
DUMPH    JSR  PRBYTE   ;Display byte as a HEX value
DUMPA    JSR  SPC2     ;Send 2 [SPACE] to terminal
         INC  IDY      ;Increment line byte counter
         LDY  IDY
         CPY  #$10
         BNE  GETBYT   ;LOOP back to GETBYT IF < $10 bytes have been displayed
         JSR  CROUT    ; ELSE, send CR,LF to terminal
         CLC           ;Add $10 to line base address, save result in
         LDA  INDEX    ; INDEX,INDEXH and TEMP2,TEMP2H
         ADC  #$10
         STA  INDEX
         STA  TEMP2
         BCC  ENDUMP   ;GOTO ENDUMP IF low byte ADD did not produce a CARRY
         INC  INDEXH   ; ELSE, increment high byte
         LDA  INDEXH
         STA  TEMP2H
ENDUMP   INC  IDX      ;Increment line counter
         LDX  IDX
         CPX  #$10     ;LOOP back to DLINE IF < $10 lines have been displayed
         BNE  DLINE
XITDMP   JSR  GLINE    ; ELSE, Send horizontal line to terminal
;DMPGR subroutine: Send address offsets to terminal
DMPGR    JSR  CROUT
         JSR  SPC4     ;Send 4 [SPACE] to terminal
         LDA  #$0D     ;Send "adrs+" to terminal
         JSR  PROMPT
         JSR  SPC4     ;Send 4 [SPACE] to terminal
         LDX  #$00     ;Initialize line counter
         STX  IDX
MDLOOP   TXA           ;Send "00" thru "0F", separated by 2 [SPACE], to terminal  
         JSR  PRBYTE
         JSR  SPC2
         INX
         CPX  #$10
         BNE  MDLOOP
         RTS           ;Done DMPGR or MEMDMP subroutine, RETURN
;
;PRASC subroutine: Send byte in ACCUMULATOR to terminal IF it is a printable ASCII character,
; ELSE, send "." to terminal. Printable ASCII byte values = $20 through $7E
;PERIOD subroutine: Send "." to terminal
PRASC    CMP  #$7F
         BCS  PERIOD   ;GOTO PERIOD IF byte = OR > $7F
         CMP  #$20
         BCS  ASCOUT   ; ELSE, GOTO ASCOUT IF byte = OR > $20   
PERIOD   LDA  #$2E     ;  ELSE, load ACCUMULATOR with "."
ASCOUT   JMP  COUT     ;Send byte in ACCUMULATOR to terminal then done PRASC subroutine, RETURN 
;
;PRBYTE subroutine: Send 2 ASCII HEX digits to terminal which represent value in ACCUMULATOR
PRBYTE   JSR  SAVREGS  ;Save ACCUMULATOR, X,Y REGISTERS on STACK
         JSR  BN2ASC   ;Convert byte in ACCUMULATOR to 2 ASCII HEX digits
         JSR  COUT     ;Send most significant HEX digit to terminal
         TYA
         JSR  COUT     ;Send least significant HEX digit to terminal
         JSR  RESREGS  ;Restore ACCUMULATOR, X,Y REGISTERS from STACK
         RTS           ;Done PRBYTE subroutine, RETURN
;
;PRINDX subroutine: Send 4 ASCII HEX digits to terminal which represent value in
; 16 bit variable INDEX,INDEXH
PRINDX   PHA           ;Save ACCUMULATOR
         LDA  INDEXH   ;Display INDEX high byte
         JSR  PRBYTE
         LDA  INDEX    ;Display INDEX low byte
         JSR  PRBYTE
         PLA           ;Restore ACCUMULATOR
         RTS           ;Done PRINDX subroutine, RETURN
;
;PROMPT subroutine: Send indexed text string to terminal. Index is ACCUMULATOR,
; strings buffer address is stored in variable PROLO, PROHI
PROMPT   JSR  SAVREGS  ;Save ACCUMULATOR, X,Y REGISTERS on STACK
         TAX           ;Initialize string counter from ACCUMULATOR
         LDY  #$00     ;Initialize byte counter
FIND     LDA  (PROLO),Y ;Read indexed byte from buffer
         BEQ  ZERO     ;GOTO ZERO IF byte = $00
NEXT     INY           ; ELSE, increment byte counter
         JMP  FIND     ;LOOP back to FIND
ZERO     DEX           ;Decrement string counter
         BNE  NEXT     ;LOOP back to NEXT IF string counter <> 0
         INY           ; ELSE, increment byte counter
;Send indexed string to terminal
STRINGL  LDA  (PROLO),Y ;Read indexed byte from buffer
         BEQ  QUITP    ;GOTO QUITP IF byte = $00
         JSR  COUT     ; ELSE, send byte to terminal
         INY           ;Increment byte counter
         BNE  STRINGL  ;LOOP back to STRINGL: always branch
QUITP    JSR  RESREGS  ;Restore ACCUMULATOR, X,Y REGISTERS from STACK
         RTS           ;Done PROMPT subroutine, RETURN
;
;READ subroutine: Read from address pointed to by INDEX,INDEXH
READ     LDY  #$00
         LDA  (INDEX),Y
         RTS           ;Done READ subroutine, RETURN
;
;RDCHAR subroutine: Wait for a keystroke then clear bit 7 of keystroke value.
; IF keystroke is a lower-case alphabetical, convert it to upper-case
RDCHAR   JSR  CHIN     ;Request keystroke input from terminal
         AND  #$7F     ;Clear bit 7 of keystroke value
         CMP  #$61
         BCC  AOK      ;GOTO AOK IF keystroke value < $61: a control code, upper-case alpha or numeral
         SEC           ; ELSE, subtract $20: convert to upper-case
         SBC  #$20
AOK      RTS           ;Done RDCHAR subroutine, RETURN
;
;The following subroutine (RDLINE) is derivative of published material:
; (pp. 418-424)
; "6502 assembly language subroutines"
; By Lance A. Leventhal, Winthrope Saville
; copyright 1982 by McGraw-Hill
; ISBN: 0-931988-59-4
;RDLINE subroutine: Setup a keystroke input buffer then store keystrokes in buffer
; until [RETURN] key it struck. Lower-case alphabeticals are converted to upper-case.
;Call with:
; ACCUMULATOR = buffer address high byte
; Y REGISTER  = buffer address low byte
; X REGISTER  = buffer length
;IF variable LOKOUT = $00 then allow all keystrokes.
;IF variable LOKOUT = $FF then allow only ASCII DECIMAL numeral input: 0123456789 
;IF variable LOKOUT = $01 through $FE then allow only valid ASCII HEX numeral input: 0123456789ABCDEF
;[BACKSPACE] key removes keystrokes from buffer.
;[ESCAPE] key aborts RDLINE then re-enters monitor. 
RDLINE   STA  BUFADRH  ;Store buffer address high byte
         STY  BUFADR   ;Store buffer address low byte
         STX  BUFLEN   ;Store buffer length   
         LDA  #$00     ;Initialize RDLINE buffer index: # of keystrokes stored in buffer = 0
         STA  BUFIDX
RDLOOP   JSR  RDCHAR   ;Request keystroke input from terminal, convert lower-case Alpha. to upper-case
         CMP  #$1B     ;GOTO NOTESC IF keystroke <> [ESCAPE]
         BNE  NOTESC
         JMP  NMON     ; ELSE, abort RDLINE, GOTO NMON: re-enter monitor
;****Replace above JMP NMON with NOP, NOP, NOP to DISABLE********************************************************
; [ESCAPE] key function during RDLINE keystroke input,
; including HEXIN subroutine digit input
; This will prevent application users from inadvertently
; entering SyMon monitor****
NOTESC   CMP  #$0D     ;GOTO EXITRD IF keystroke = [RETURN]
         BEQ  EXITRD
         CMP  #$08     ; ELSE, GOTO BACK IF keystroke = [BACKSPACE]
         BEQ  BACK
         LDX  LOKOUT   ; ELSE, GOTO FULTST IF variable LOKOUT = $00: keystroke filter is disabled 
         BEQ  FULTST
         CMP  #$30     ; ELSE, filter enabled, GOTO INERR IF keystroke value < $30: value < ASCII "0"
         BCC  INERR
         CMP  #$47     ; ELSE, GOTO INERR IF keystroke = OR > $47: value = OR > ASCII "G" ("F" + 1)
         BCS  INERR
         CMP  #$41     ; ELSE, GOTO DECONLY IF keystroke = OR > $41: value = OR > ASCII "A"
         BCS  DECONLY
         BCC  DECTEST  ; ELSE, GOTO DECTEST: keystroke < $41: value < ASCII "A"
DECONLY  CPX  #$FF     ;GOTO FULTST IF LOKOUT variable <> $FF: ASCII DECIMAL digit filter disabled
         BNE  FULTST
DECTEST  CMP  #$3A     ; ELSE, DECIMAL filter enabled, GOTO INERR IF keystroke = OR > $3A:
         BCS  INERR    ;   value = OR > ASCII ":" ("9" + 1)
FULTST   LDY  BUFIDX   ; ELSE, GOTO STRCH IF BUFIDX <> BUFLEN: buffer is not full
         CPY  BUFLEN
         BCC  STRCH
INERR    JSR  BEEP     ; ELSE, Send [BELL] to terminal
         BNE  RDLOOP   ;LOOP back to RDLOOP: always branch
STRCH    STA  (BUFADR),Y ;Store keystroke in buffer
         JSR  COUT     ;Send keystroke to terminal
         INC  BUFIDX   ;Increment buffer index
         BNE  RDLOOP   ;LOOP back to RDLOOP: always branch
EXITRD   LDX  BUFIDX   ;Copy keystroke count to X-REGISTER
         RTS           ;Done RDLINE subroutine, RETURN
;Perform a destructive [BACKSPACE]
BACK     LDA  BUFIDX   ;GOTO BSERROR IF buffer is empty
         BEQ  BSERROR
         DEC  BUFIDX   ; ELSE, decrement buffer index
         JSR  BSOUT    ;Send [BACKSPACE] to terminal
         JSR  SPC      ;Send [SPACE] to terminal
         JSR  BSOUT    ;Send [BACKSPACE] to terminal
         BNE  RDLOOP   ;LOOP back to RDLOOP: always branch
BSERROR  JSR  BEEP     ;Send [BELL] to terminal
         BEQ  RDLOOP   ;LOOP back to RDLOOP: always branch
;
;RESREGS subroutine: restore ACCUMULATOR, X,Y REGISTERS from the copy previously saved
; on the STACK by the SAVREGS subroutine
RESREGS  PLA           ;Pull RTS RETURN address high byte from STACK
         STA  RT1      ;Save RTS RETURN address high byte to memory
         PLA           ;Pull RTS RETURN address low byte from STACK
         STA  RT2      ;Save RTS RETURN address low byte to memory
         PLA           ;Pull saved X REGISTER from STACK
         TAX           ;Restore X REGISTER
         PLA           ;Pull saved Y REGISTER from STACK
         TAY           ;Restore Y REGISTER
         PLA           ;Pull saved ACCUMULATOR from STACK
         STA  STEMP    ;Save ACCUMULATOR to memory
         LDA  RT2      ;Read RTS RETURN address low byte from memory
         PHA           ;Push RTS RETURN address low byte onto STACK
         LDA  RT1      ;Read RTS RETURN address high byte from memory
         PHA           ;Push RTS RETURN address high byte onto STACK
         LDA  STEMP    ;Restore ACCUMULATOR from memory
         RTS           ;Done RESREGS subroutine, RETURN
;
;SAVREGS subroutine: save a copy of ACCUMULATOR, X,Y REGISTERS on the STACK.
;This is used in conjunction with the RESREGS subroutine 
SAVREGS  STA  STEMP    ;Save ACCUMULATOR to memory
         PLA           ;Pull RTS RETURN address high byte from STACK
         STA  RT1      ;Save RTS RETURN address high byte to memory
         PLA           ;Pull RTS RETURN address low byte from STACK
         STA  RT2      ;Save RTS RETURN address low byte to memory
         LDA  STEMP    ;Restore ACCUMULATOR from memory
         PHA           ;Push ACCUMULATOR onto STACK
         TYA           ;Push Y REGISTER onto STACK
         PHA
         TXA           ;Push X REGISTER onto STACK
         PHA
         LDA  RT2      ;Read RTS RETURN address low byte from memory
         PHA           ;Push RTS RETURN address low byte onto STACK
         LDA  RT1      ;Read RTS RETURN address high byte from memory
         PHA           ;Push RTS RETURN address high byte onto STACK
         LDA  STEMP    ;Restore ACCUMULATOR from memory
         RTS           ;Done SAVREGS subroutine, RETURN
;
;SBYTSTR subroutine: request 0 - 16 byte string from terminal, each byte followed by [RETURN].
; [ESCAPE] aborts. String will be stored in 16 byte buffer beginning at address SRCHBUFF.
; Y REGISTER holds number of bytes in buffer. This is used by monitor text/byte string
; search commands
SBYTSTR  LDY  #$00     ;Initialize index/byte counter
SBLOOP   STY  IDY      ;Save index/byte counter 
         LDA  #$02     ;Request 2 HEX digit input from terminal (byte value)
         JSR  HEXIN
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         LDY  IDY      ;Restore index/byte counter 
         CPX  #$00     ;GOTO DONESB IF no digits were entered
         BEQ  DONESB
         STA  SRCHBUFF,Y ; ELSE, Store byte in indexed buffer location
         INY           ;Increment index/byte counter
         CPY  #$10     ;LOOP back to SBLOOP IF index/byte counter < $10
         BNE  SBLOOP
DONESB   RTS           ; ELSE, done SBYTSTR, RETURN
;
;SENGINE subroutine: Scan memory range $0300 through $FFFF for exact match to string
; contained in buffer SRCHBUFF (1 to 16 bytes/characters). Display address of first
; byte/character of each match found until the end of memory is reached.
; This is used by monitor text/byte string search commands 
SENGINE  CPY  #$00     ;GOTO DUNSENG IF SRCHBUFF buffer is empty
         BEQ  DUNSENG
         STY  IDY      ; ELSE, save buffer byte/character count
         LDA  #$03     ;Initialize memory address pointer to $0300: skip over $0000 through $0200
         STA  INDEXH
         LDY  #$00     ; and initialize (zero) memory address index
         STY  INDEX
SENGBR2  LDX  #$00     ;Initialize buffer index: point to first byte/character in buffer
SENGBR3  LDA  (INDEX),Y ;Read current memory location
         CMP  SRCHBUFF,X ;GOTO SENGBR1 IF memory matches indexed byte/character in buffer 
         BEQ  SENGBR1
         LDX  #$00     ; ELSE, initialize buffer index: point to first byte/character in string 
         LDA  (INDEX),Y ;Read current memory location
         CMP  SRCHBUFF,X ;GOTO SENGBR2 IF memory matches indexed byte/character in buffer 
         BEQ  SENGBR2
         JSR  SINCPTR  ; ELSE, increment memory address pointer, test for end of memory 
         JMP  SENGBR2  ;LOOP back to SENGBR2
SENGBR1  JSR  SINCPTR  ;Increment memory address pointer, test for end of memory 
         INX           ;Increment buffer index
         CPX  IDY      ;LOOP back to SENGBR3 IF buffer index <> memory index
         BNE  SENGBR3
         SEC           ; ELSE, subtract buffer index from memory address pointer:
         LDA  INDEX    ;  (point to first byte of matching string found in memory)  
         SBC  IDY
         STA  INDEX
         LDA  INDEXH
         SBC  #$00
         STA  INDEXH
         LDA  #$0A     ;Send "found " to terminal
         JSR  PROMPT
         LDA  #$0C     ;Send "at: $" to terminal
         JSR  PROMPT
         LDA  INDEXH   ;Display address of matching string
         JSR  PRBYTE
         LDA  INDEX
         JSR  PRBYTE
         LDA  #$0E     ;Send "N=Next? " to terminal
         JSR  PROMPT
         JSR  CHIN     ;Request a keystroke from terminal
         AND  #$DF     ;Convert lower case Alpha. to upper case
         CMP  #$4E     ;GOTO DUNSENG IF keystroke <> "N"
         BNE  DUNSENG
         JSR  SINCPTR  ;ELSE, increment memory address pointer, test for end of memory  
         JMP  SENGBR2  ;LOOP back to SENGBR2
DUNSENG  LDA  INCNT    ;Remove last keystroke from keystroke input buffer
         STA  OUTCNT
         RTS           ;Done SENGINE subroutine, RETURN
;Increment memory address pointer. If pointer high byte = 00 (end of memory),
; send "not found" to terminal then return to monitor 
SINCPTR  JSR  INCINDEX ;Increment memory address pointer  
         CMP  #$00     ;GOTO DUNSINC IF memory address pointer high byte <> 00
         BNE  DUNSINC
         LDA  #$0B     ; ELSE, send "not " to terminal
         JSR  PROMPT
         LDA  #$0A     ;Send "found " to terminal
         JSR  PROMPT
         LDA  INCNT    ;Remove last keystroke from keystroke input buffer
         STA  OUTCNT
         JMP  NMON     ;Done SRCHBYT or SRCHTXT command, GOTO NMON: return to monitor 
DUNSINC  RTS           ;Done SINCPTR subroutine, RETURN 
;
;SET subroutine: Set TIMER duration from X REGISTER then call TIMER.  
;TIMER subroutine: Perform a long delay. Duration is SETIM * (DELLO * DELHI)
SET      STX  SETIM    ;Store duration value in SETIM variable 
TIMER    JSR  SAVREGS  ;Save ACCUMULATOR, X,Y REGISTERS on STACK
         LDX  SETIM    ;Copy SETIM to TEMP variable: TIMER multiplier
         STX  TEMP
TIMERL   JSR  DELAY2   ;Perform a medium delay. Duration is DELLO * DELHI 
         DEC  TEMP     ;Decrement TIMER multiplier
         BNE  TIMERL   ;LOOP back to TIMERL IF TIMER multiplier <> 0
         JSR  RESREGS  ; ELSE, restore ACCUMULATOR, X,Y REGISTERS from STACK
         RTS           ;Done SET or TIMER subroutine, RETURN
;
;SETUP subroutine: Request HEX address input from terminal 
SETUP    JSR  COUT     ;Send command keystroke to terminal
         JSR  SPC      ;Send [SPACE] to terminal
;Request a 0-4 digit HEX address input from terminal
         JSR  HEXIN4   ; result in variable INDEX,INDEXH
         LDA  #$3A     ;Send ":" to terminal
         JSR  COUT
         JMP  DOLLAR   ;Send "$" to terminal then done SETUP subroutine, RETURN
;
;SPC subroutines: Send [SPACE] to terminal 1,2 or 4 times
SPC4     JSR  SPC2     ;Send 4 [SPACE] to terminal
SPC2     JSR  SPC      ;Send 2 [SPACE] to terminal
SPC      PHA           ;Save ACCUMULATOR
         LDA  #$20     ;Send [SPACE] to terminal
         JSR  COUT
         PLA           ;Restore ACCUMULATOR
         RTS           ;Done SPC(n) subroutine, RETURN
;
;STXTSTR subroutine: request 1 - 16 character text string from terminal, followed by [RETURN].
; [ESCAPE] aborts, [BACKSPACE] erases last keystroke. String will be stored in 
; 16 byte buffer beginning at address SRCHBUFF. Y REGISTER holds number of characters in buffer
; This is used by monitor text/byte string search commands 
STXTSTR  LDY  #$00     ;Initialize index/byte counter
STLOOP   JSR  CHIN     ;Request keystroke input from terminal
         CMP  #$0D     ;GOTO STBR1 IF keystroke = [RETURN]
         BEQ  STBR1
         CMP  #$1B     ; ELSE, GOTO STBR2 IF keystroke <> [ESCAPE]
         BNE  STBR2
         LDY  #$00     ; ELSE, indicate that there are no keystrokes to process
STBR1    RTS           ;Done STXTSTR subroutine, RETURN
STBR2    CMP  #$08     ;GOTO STBR3 IF keystroke <> [BACKSPACE]
         BNE  STBR3
         TYA           ; ELSE, LOOP back to STLOOP IF buffer is empty  
         BEQ  STLOOP
         JSR  BSOUT    ; ELSE, send [BACKSPACE] to terminal
         DEY           ;Decrement index/byte counter
         JMP  STLOOP   ;LOOP back to STLOOP
STBR3    STA  SRCHBUFF,Y ;Store character in indexed buffer location
         JSR  COUT     ;Send character to terminal
         INY           ;Increment index/byte counter
         CPY  #$10     ;LOOP back to STLOOP IF index/byte counter < $10
         BNE  STLOOP
         RTS           ; ELSE, done STXTSTR subroutine, RETURN
;
;DECIN subroutine: request 1 - 10 DECIMAL digit input from terminal, followed by [RETURN].
; [ESCAPE] aborts, [BACKSPACE] erases last keystroke. 
; Convert input to BCD and HEX then store both results as follows: 
; Converted 10 digit (5 byte) BCD value will be contained in variables DEC0AND1 through DEC8AND9:
;  DEC0AND1 ($E5) Two most significant BCD digits
;  DEC2AND3 ($E6)
;  DEC4AND5 ($E7)
;  DEC6AND7 ($E8)
;  DEC8AND9 ($E9) Two least significant BCD digits
; Converted 8 digit (4 byte) HEX value will be contained in variables HEX0AND1 through HEX6AND7:
;  HEX0AND1 ($E1) Two most significant HEX digits
;  HEX2AND3 ($E2)
;  HEX4AND5 ($E3)
;  HEX6AND7 ($E4) Two least significant HEX digits
; NOTE1: If a DECIMAL value greater than 4,294,967,295 ($FFFFFFFF) is entered,
;  1 or 2 asterisks (*) will be sent to the terminal following the inputted digits.
;  This is to indicate that an overflow in the HEX accumulator has occured.
;  (the BCDTOHEX subroutine's HEX accumulator "rolls over" to zero when that value is exceeded)
;  An overflow condition does NOT affect the BCD value stored.
; NOTE2: This subroutine is not used by SyMon; it is here for user purposes, if needed.    
DECIN   JSR  DECINPUT  ;Request 1 - 10 DECIMAL digit input from terminal
        JSR  ASCTODEC  ;Convert ASCII DECIMAL digits to BCD
        JSR  BCDTOHEX  ;Convert a 1-10 digit BCD value to a 1-8 digit HEX value
        RTS            ;Done DECIN subroutine, RETURN
;
;
;******************************
;* Monitor command processors *
;******************************
;
;[invalid] command: No command assigned to keystroke. This handles unassigned keystrokes
ERR      JSR  BEEP     ;Send ASCII [BELL] to terminal
         PLA           ;Remove normal return address because we don't 
         PLA           ; want to send a monitor prompt in this case
         JMP  CMON     ;GOTO CMON re-enter monitor
;
;[A] ACCUMULATOR command: Display in HEX then change ACCUMULATOR preset/result
ARG      LDA  #$05     ;Send "Areg:$" to terminal
         JSR  PROMPT
         LDA  ACCUM    ;Read ACCUMULATOR preset/result
         JSR  CHREG    ;Display preset/result, request HEX byte input from terminal
         LDX  SCNT     ;GOTO NCAREG IF no digits were entered
         BEQ  NCAREG
         STA  ACCUM    ; ELSE, Write to ACCUMULATOR preset/result
NCAREG   RTS           ;Done ARG command, RETURN
;
;[B] BYTE command: Display in HEX the ACCUMULATOR preset/result
BYTE     JSR  DOLLAR   ;Send "$" to terminal
         LDA  ACCUM    ;Read ACCUMULATOR preset/result
         JMP  PRBYTE   ;Display HEX value read then done BYTE command, RETURN
;
;[C] CHANGE command: Display in HEX then change the contents of a specified memory address 
CHANGE   JSR  SETUP    ;Request HEX address input from terminal
         JSR  READ     ;Read specified address
         JSR  PRBYTE   ;Display HEX value read
CHANGEL  LDA  #$08     ;Send 3 non-destructive [BACKSPACE] to terminal
         JSR  COUT3
;Request a HEX byte input from terminal
         JSR  HEXIN2   ; result in ACCUMULATOR, X-reg and variable SCNT = # digits entered 
         LDY  SCNT     ;Exit CHANGE command IF no digits were entered
         BEQ  XITWR
         LDY  #$00     ; ELSE, Store entered value at current INDEX address
         STA  (INDEX),Y
         CMP  (INDEX),Y ;GOTO CHOK IF stored value matches ACCUMULATOR
         BEQ  CHOK
         LDA  #$3C     ; ELSE, Send "<" to terminal
         JSR  COUT
         LDA  #$3F     ;Send "?" to terminal
         JSR  COUT
CHOK     INC  INDEX    ;Increment INDEX address
         BNE  PRNXT
         INC  INDEXH
PRNXT    JSR  SPC2     ;Send 2 [SPACE] to terminal
         JSR  READ     ;Read from address pointed to by INDEX,INDEXH
         JSR  PRBYTE   ;Display HEX value read
         JMP  CHANGEL  ;LOOP back to CHANGEL: continue CHANGE command
XITWR    RTS           ;Done CHANGE command, RETURN
;
;[D] HEX DUMP command: Display in HEX the contents of 256 consecutive memory addresses 
MDUMP    JSR  SETUP    ;Request HEX address input from terminal
         LDA  #$00     ;Select HEX dump mode
         JMP  MEMDMP   ;Go perform memory dump then done MDUMP command, RETURN
;
;[E] EXAMINE command: Display in HEX the contents of a specified memory address
EXAMINE  JSR  SETUP    ;Request HEX address input from terminal
         JSR  READ     ;Read from specified address
         JMP  PRBYTE   ;Go display HEX value read then done EXAMINE command, RETURN
;
;[F] MFILL command: Fill a specified memory range with a specified value
MFILL    LDA  #$FE     ;Point to prompt strings array beginning at $FE00
         STA  PROHI
         LDA  #$02     ;Send CR,LF, "fill start: " to terminal
         JSR  PROMPT
         JSR  HEXIN4   ;Request 4 digit HEX value input from terminal. Result in variable INDEX,INDEXH
         LDX  SCNT     ;GOTO DUNFILL IF no digits were entered
         BEQ  DONEFILL 
         LDA  INDEX    ; ELSE, copy INDEX,INDEXH to TEMP2,TEMP2H: memory fill start address
         STA  TEMP2
         LDA  INDEXH
         STA  TEMP2H
         LDA  #$03     ;Send CR,LF, "length: " to terminal
         JSR  PROMPT
         JSR  HEXIN4   ;Request 4 digit HEX value input from terminal. Result in variable INDEX,INDEXH: memory
                       ; fill length
         LDX  SCNT     ;GOTO DUNFILL IF no digits were entered
         BEQ  DONEFILL
         LDA  #$04     ; ELSE, send CR,LF, "value: " to terminal
         JSR  PROMPT
         JSR  HEXIN2   ;Request 2 digit HEX value input from terminal. Result in ACCUMULATOR: fill value
         LDX  SCNT     ;GOTO DUNFILL IF no digits were entered
         BEQ  DONEFILL
         PHA           ;Save fill value on STACK
         LDA  #$01     ;Send CR,LF, "Esc key exits, any other to proceed" to terminal 
         JSR  PROMPT
         JSR  CHIN     ;Request keystroke input from terminal
         CMP  #$1B     ;GOTO QUITFILL IF keystroke = [ESCAPE]
         BEQ  QUITFILL
         PLA           ; ELSE, pull fill value from STACK
USERFILL LDX  INDEXH   ;Copy INDEXH to X REGISTER: X = length parameter page counter
         BEQ  FILEFT   ;GOTO FILEFT IF page counter = $00
         LDY  #$00     ; ELSE, initialize page byte address index
PGFILL   STA  (TEMP2),Y ;Store fill value at indexed current page address
         INY           ;Increment page byte address index 
         BNE  PGFILL   ;LOOP back to PGFILL IF page byte address index <> $00
         INC  TEMP2H   ; ELSE, Increment page address high byte 
         DEX           ;Decrement page counter
         BNE  PGFILL   ;LOOP back to PGFILL IF page counter <> $00 
FILEFT   LDX  INDEX    ; ELSE, copy INDEX to X REGISTER: X = length parameter byte counter 
         BEQ  DONEFILL ;GOTO DONEFILL IF byte counter = $00
         LDY  #$00     ; ELSE, initialize page byte address index
FILAST   STA  (TEMP2),Y ;Store fill value at indexed current page address
         INY           ;Increment page byte address index
         DEX           ;Decrement byte counter
         BNE  FILAST   ;LOOP back to FILAST IF byte counter <> $00
DONEFILL RTS           ; ELSE, done MFILL command, RETURN
QUITFILL PLA           ;Pull saved fill value from STACK: value not needed, discard
         RTS           ;Done MFILL command, RETURN
;
;[G] GO command: Begin executing program code at a specified address
GO       JSR  SETUP    ;Request HEX address input from terminal
         JSR  CROUT    ;Send CR,LF to terminal
         LDA  INDEX    ;Transfer specified address to monitor command processor 
         STA  COMLO    ; address pointer low byte
         LDA  INDEXH
         STA  COMHI    ;  hi byte
CNTLUGO  TSX           ;Save the monitor's STACK POINTER in memory
         STX  POINTER
;Preload all 6502 MPU registers from monitor's preset/result variables
         LDX  SREG     ;Load STACK POINTER preset
         TXS
         LDA  PREG     ;Load PROCESSOR STATUS REGISTER preset
         PHA
         LDA  ACCUM    ;Load ACCUMULATOR preset
         LDX  XREG     ;Load X-REGISTER preset
         LDY  YREG     ;Load Y-REGISTER preset
         PLP
;Call user program code as a subroutine
         JSR  DOCOM
;Store all 6502 MPU registers to monitor's preset/result variables: store results
         PHP
         STA  ACCUM    ;Store ACCUMULATOR result
         STX  XREG     ;Store X-REGISTER result
         STY  YREG     ;Store Y-REGISTER result
         PLA
         STA  PREG     ;Store PROCESSOR STATUS REGISTER result
         TSX
         STX  SREG     ;Store STACK POINTER result
         LDX  POINTER  ;Restore the monitor's STACK POINTER
         TXS
         CLD           ;Put 6502 in binary arithmatic mode
         RTS           ;Done GO command, RETURN
;
;[H] HEXTODEC command: Convert an entered 1-to-8 digit HEX value to
; a 1-to-10 digit BCD value then display DECIMAL result  
HEXTODEC LDA  #$06     ;Send "HEX: $" to terminal
         JSR  PROMPT
         JSR  HEXINPUT ;Request 0 to 8 digit HEX value from terminal then convert ASCII HEX digits to HEX
         LDA  BUFIDX   ;GOTO HTDDONE IF BUXIDX = 0: no digits were entered
         BEQ  HTDDONE
         JSR  CROUT    ; ELSE send CR,LF to terminal
         LDA  #$08     ;Send "DEC: " to terminal
         JSR  PROMPT
         JSR  HEXTOBCD ;Convert 8 digit HEX value to a 10 digit BCD value
         JSR  BCDOUT   ;Convert 10 digit BCD value to 10 ASCII DECIMAL digits,
                       ; send result to terminal 
HTDDONE  RTS           ;Done HEXTODEC command, RETURN
;
;[I] INPUT command: Get a keystroke then write it to ACCUMULATOR preset/result 
INPUT    JSR  CHIN     ;Request keystroke input from terminal
         STA  ACCUM    ;Write to ACCUMULATOR preset/result
         RTS           ;Done INPUT command, RETURN
;
;[J] DECTOHEX command: Convert an entered 1-to-10 digit DECIMAL value to
; a 1-to-8 digit HEX value then display HEX result
DECTOHEX JSR  CROUT    ;Send CR,LF to terminal
         LDA  #$08     ;Send "DEC: " to terminal
         JSR  PROMPT
         JSR  DECINPUT ;Request 0 to 10 digit decimal numeral input from terminal
         LDA  BUFIDX   ;GOTO DONEDEC IF BUFIDX = 0: no digits were entered
         BEQ  DONEDEC
         LDA  #$06     ; ELSE, send "HEX: " to terminal
         JSR  PROMPT
         JSR  ASCTODEC ;Convert ASCII digits to BCD
         JSR  BCDTOHEX ;Convert BCD digits to HEX
         JSR  HEXOUT   ;Convert HEX digits to ASCII HEX digits, send result to terminal
DONEDEC  RTS           ;Done DECTOHEX command, RETURN
;
;[K] LOCATE BYTE STRING command: search memory for an entered byte string.
;Memory range scanned is $0300 through $FFFF (specified in SENGINE subroutine)
; (SRCHTXT [L] command uses this, enters at SRCHRDY) 
SRCHBYT  LDA  #$FE     ;Point to prompt strings located at $FE00
         STA  PROHI
         LDA  #$07     ;Send CR,LF, "Find " to terminal
         JSR  PROMPT
         LDA  #$09     ;Send "bytes: " to terminal
         JSR  PROMPT
         JSR  SBYTSTR  ;Request byte string input from terminal
SRCHRDY  CPY  #$00     ;GOTO SBDONE IF no bytes in buffer
         BEQ  SBDONE
         LDA  #$0D     ; ELSE, send "Searching.." to terminal
         JSR  PROMPT
         JSR  SENGINE  ;Perform search for string in memory
SBDONE   JMP  NMON     ;Done SRCHBYT or SRCHTXT command, GOTO NMON: return to monitor 
;
;[L] LOCATE TEXT STRING command: search memory for an entered text string.
;Memory range scanned is $0300 through $FFFF (specified in SENGINE subroutine)
SRCHTXT  LDA  #$FE     ;Point to prompt strings located at $FE00
         STA  PROHI
         LDA  #$07     ;Send CR,LF, "Find " to terminal
         JSR  PROMPT
         LDA  #$08     ;Send "text: " to terminal
         JSR  PROMPT
         JSR  STXTSTR  ;Request text string input from terminal
         JMP  SRCHRDY  ;GOTO SRCHRDY (part of SRCHBYT command)
;
;The following command processor (MOVER) is derivative of published material:
; (pp. 197-203)
; "6502 assembly language subroutines"
; By Lance A. Leventhal, Winthrope Saville
; copyright 1982 by McGraw-Hill
; ISBN: 0-931988-59-4
;[M] MOVER command: Move (copy) specified source memory area to specified destination memory area
MOVER    LDA  #$FE     ;Point to prompt strings array beginning at $FE00
         STA  PROHI
         LDA  #$05     ;Send CR,LF, "move source: " to terminal
         JSR  PROMPT
         JSR  HEXIN4   ;Request 4 digit HEX value input from terminal. Result in variable INDEX,INDEXH: memory
                       ; move source address
         LDX  SCNT     ;GOTO QUITMV IF no digits were entered
         BEQ  QUITMV
         STA  TEMP3    ; ELSE, store source address in variable TEMP3,TEMP3H
         LDA  INDEXH
         STA  TEMP3H
         LDA  #$06     ;Send CR,LF, "destination: " to terminal
         JSR  PROMPT
         JSR  HEXIN4   ;Request 4 digit HEX value input from terminal. Result in variable INDEX,INDEXH: memory
                       ; move destination address
         LDX  SCNT     ;GOTO QUITMV IF no digits were entered
         BEQ  QUITMV
         STA  TEMP2    ; ELSE, store destination address in variable TEMP2,TEMP2H
         LDA  INDEXH
         STA  TEMP2H
         LDA  #$03     ;Send CR,LF, "length: " to terminal
         JSR  PROMPT
         JSR  HEXIN4   ;Request 4 digit HEX value input from terminal. Result in variable INDEX,INDEXH: memory
                       ; move length
         LDX  SCNT     ;GOTO QUITMV IF no digits were entered
         BEQ  QUITMV
         LDA  #$01     ;Send CR,LF, "Esc key exits, any other to proceed" to terminal
         JSR  PROMPT
         JSR  CHIN     ;Request keystroke input from terminal
         CMP  #$1B     ;GOTO QUITMV IF keystroke = [ESCAPE]
         BEQ  QUITMV
         LDA  TEMP2    ; ELSE, GOTO RIGHT IF source memory area is above destination memory area
         SEC           ;  AND overlaps it (destination address - source address) < length of move
         SBC  TEMP3
         TAX           ;   X REGISTER = destination address low byte - source address low byte
         LDA  TEMP2H
         SBC  TEMP3H
         TAY           ;   Y REGISTER = destination address high byte - (source address high byte + borrow)
         TXA
         CMP  INDEX    ;   produce borrow IF (destination address low - source address low) < destination low  
         TYA
         SBC  INDEXH   ;   produce borrow IF ((destination address high - source address high) + borrow) < dest. high
         BCC  RIGHT    ;   Branch if a borrow was produced (unmoved source memory overwrite condition detected)
;Move memory block first byte to last byte 
         LDY  #$00     ; ELSE, initialize page byte index
         LDX  INDEXH   ;Copy length high byte to X REGISTER: page counter
         BEQ  MVREST   ;GOTO MVREST IF move length is < $100 bytes
MVPGE    LDA  (TEMP3),Y ; ELSE, move (copy) byte from indexed source to indexed destination memory
         STA  (TEMP2),Y
         INY           ;Increment page byte index
         BNE  MVPGE    ;LOOP back to MVPAGE IF page byte index <> 0: loop until entire page is moved  
         INC  TEMP3H   ; ELSE, increment source page address
         INC  TEMP2H   ;Increment destination page address
         DEX           ;Decrement page counter
         BNE  MVPGE    ;LOOP back to MVPAGE IF page counter <> 0: there are more full pages to move
                       ; ELSE,(page byte index (Y REGISTER) = 0 at this point)
MVREST   LDX  INDEX    ;Copy length parameter low byte to X REGISTER: byte counter
         BEQ  QUITMV   ;GOTO QUITMV IF byte counter = 0: there are no bytes left to move
REST     LDA  (TEMP3),Y ; ELSE, move (copy) byte from indexed source to indexed destination memory
         STA  (TEMP2),Y
         INY           ;Increment page byte index
         DEX           ;Decrement byte counter
         BNE  REST     ;LOOP back to REST IF byte counter <> 0: loop until all remaining bytes are moved 
QUITMV   RTS           ; ELSE, done MOVER command, RETURN
;Move memory block last byte to first byte (avoids unmoved source memory overwrite in certain source/dest. overlap) 
RIGHT    LDA  INDEXH   ;Point to highest address page in source memory block
         CLC           ;  source high byte = (source high byte + length high byte) 
         ADC  TEMP3H
         STA  TEMP3H
         LDA  INDEXH   ;Point to highest address page in destination memory block
         CLC           ;  destination high byte = (destination high byte + length high byte) 
         ADC  TEMP2H
         STA  TEMP2H
         LDY  INDEX    ;Copy length low byte to Y REGISTER: page byte index
         BEQ  MVPAG    ;GOTO MVPAG IF page byte index = 0: no partial page to move
RT       DEY           ; ELSE, decrement page byte index
         LDA  (TEMP3),Y ;Move (copy) byte from indexed source to indexed destination memory
         STA  (TEMP2),Y
         CPY  #$00     ;LOOP back to RT IF page byte index <> 0: loop until partial page is moved
         BNE  RT
MVPAG    LDX  INDEXH   ; ELSE, copy length high byte to X REGISTER: page counter
         BEQ  QUITMV   ;GOTO QUITMV IF page counter = 0: no full pages left to move
RDEC     DEC  TEMP3H   ; ELSE, decrement source page address
         DEC  TEMP2H   ;Decrement destination page address
RMOV     DEY           ;Decrement page byte index
         LDA  (TEMP3),Y ;Move (copy) byte from indexed source to indexed destination memory
         STA  (TEMP2),Y
         CPY  #$00     ;LOOP back to RMOV IF page byte index <> 0: loop until entire page is moved
         BNE  RMOV
         DEX           ; ELSE, decrement page counter
         BNE  RDEC     ;LOOP back to RDEC IF page counter <> 0: loop until all full pages are moved
         RTS           ; ELSE, done MOVER command, RETURN
;
;[N] NEXT/NOTHING command: [N] is reserved and trapped by the monitor command keystroke processor.
; It produces the same result as when an invalid key is struck:
;  Send ASCII [BELL] to terminal then RETURN to monitor. 
;
;[O] OUTPUT command: Send byte in ACCUMULATOR preset/result to terminal
OUTPUT   LDA  ACCUM    ;Read ACCUMULATOR preset/result
         JMP  COUT     ;Send byte to terminal then done OUTPUT command, RETURN
;
;[P] PROCESSOR STATUS command: Display then change PPOCESSOR STATUS preset/result
PRG      LDA  #$14     ;Send "processor status:$" to terminal
         JSR  PROMPT
         LDA  PREG     ;Read PROCESSOR STATUS preset/result
         JSR  CHREG    ;Display preset/result, request HEX byte input from terminal
         LDX  SCNT     ;GOTO NCREG IF no digits were entered
         BEQ  NCPREG
         STA  PREG     ; ELSE, write entered byte to PROCESSOR STATUS preset/result
NCPREG   RTS           ;Done PRG command, RETURN
;
;[R] REGISTERS command: Display contents of all preset/result memory locations
PRSTAT   JSR  CROUT    ;Send CR,LF to terminal
         LDX  #$00     ;Initialize register index
NXTREG   LDA  REGARA,X ;Send indexed register character (A,X,Y,S,P) to terminal
         JSR  COUT
;Send indexed preset/result HEX value to terminal
         LDA  #$3D     ;Send "=" to terminal
         JSR  COUT
         JSR  DOLLAR   ;Send "$" to terminal
;Load ACCUMULATOR from indexed preset/result
         CPX  #$00
         BNE  XR
         LDA  ACCUM    ;ACCUMULATOR preset/result
XR       CPX  #$01
         BNE  YR
         LDA  XREG     ;X-REGISTER preset/result
YR       CPX  #$02
         BNE  SR
         LDA  YREG     ;Y-REGISTER preset/result
SR       CPX  #$03
         BNE  PR
         LDA  SREG     ;STACK POINTER preset/result
PR       CPX  #$04
         BNE  XITR
         LDA  PREG     ;PROCESSOR STATUS preset/result
XITR     JSR  PRBYTE   ;Send preset/result value to terminal
         JSR  SPC2     ;Send 2 [SPACE] to terminal 
         CPX  #$04     ;Increment index then LOOP back to NXTREG IF index <> 4 
         BEQ  ARONEG
         INX
         JMP  NXTREG
ARONEG   INX           ; ELSE, Send "-> NEG" to terminal
         CPX  #$0B
         BEQ  BIT7
         LDA  REGARA,X
         JSR  COUT
         JMP  ARONEG
BIT7     JSR  PRBIT    ;Send "0" or "1"  to terminal depending on indexed bit condition
MNSTR    LDY  #$03     ;Initialize bit name character pointer
MNCHAR   LDA  REGARA,X ;Send indexed 3 character bit name to terminal 
         JSR  COUT
         INX
         DEY
         BNE  MNCHAR
         JSR  PRBIT    ;Send "0" or "1" to terminal depending on indexed bit condition 
         CPX  #$1D     ;LOOP back to MNSTR IF all PROCESSOR STATUS preset/result bits
         BNE  MNSTR    ; have not been displayed
         RTS           ;  ELSE, Done PRSTAT command, RETURN
;PRBIT subroutine: Send "0" or "1" to terminal depending on condition of indexed bit in
; PROCESSOR STATUS preset/result 
PRBIT    LDA  #$3A     ;Send ":" to terminal
         JSR  COUT
         LDA  PREG     ;Read PROCESSOR STATUS preset/result
;Select number of shifts required to isolate indexed bit
         CPX  #$0B
         BEQ  LS1
         CPX  #$0E
         BEQ  LS2
         CPX  #$11
         BEQ  LS4
         CPX  #$14
         BEQ  LS5
         CPX  #$17
         BEQ  LS6
         CPX  #$1A
         BEQ  LS7
;Left-shift indexed bit into CARRY flag
         ASL  A        ;Bit 0 CARRY
LS7      ASL  A        ;Bit 1 ZERO
LS6      ASL  A        ;Bit 2 INTERRUPT REQUEST
LS5      ASL  A        ;Bit 3 DECIMAL
LS4      ASL  A        ;Bit 4 BREAK
         ASL  A        ;Bit 5 (This bit is undefined in the 6502 PROCESSOR STATUS REGISTER) 
LS2      ASL  A        ;Bit 6 OVERFLOW
LS1      ASL  A        ;Bit 7 NEGATIVE
         BCS  BITSET   ;Send "0" to terminal IF bit is clear
         LDA  #$30
         BNE  RDONE
BITSET   LDA  #$31     ; ELSE, Send "1" to terminal
RDONE    JSR  COUT
         JMP  SPC      ;Send [SPACE] to terminal then done PRBIT subroutine, RETURN
REGARA   .DB  "AXYSP"
         .DB  "-> NEGOVRBRK"
         .DB  "DECIRQZERCAR "
;
;[S] STACK POINTER command: Display then change STACK POINTER preset/result
SRG      LDA  #$10     ;Send "stack pointer:$" to terminal
         JSR  PROMPT
         LDA  SREG     ;Read STACK POINTER preset/result
         JSR  CHREG    ;Display preset/result, request HEX byte input from terminal
         LDX  SCNT     ;GOTO NCSREG IF no digits were entered
         BEQ  NCSREG
         STA  SREG     ; ELSE, write entered byte to STACK POINTER preset/result
NCSREG   RTS           ;Done SRG command, RETURN
;
;[T] TEXT DUMP command: Display in ASCII the contents of 256 consecutive memory addresses 
ADUMP    JSR  SETUP    ;Request HEX address input from terminal
         LDA  #$01     ;Select ASCII dump mode
         JMP  MEMDMP   ;Go perform memory dump then done ADUMP command, RETURN
;
;[U] UPLOAD command: send a formatted ASCII HEX file to terminal.
; Convert file from binary to ASCII HEX, send ASCII HEX characters to terminal.
; The first character sent will be a dollar sign ($), followed by four
; ASCII HEX digits then a [SPACE]. These 4 digits represent the address from
; which it will begin reading/converinng/sending file data. What follows is the
; ASCII HEX representations of the binary file data, each byte being represented
; by TWO ASCII HEX digits, each byte being separated by a [SPACE].
; After 16 ASCII HEX BYTES have been sent, a new line will be started:
; a [RETURN], [LINEFEED] followed by the next address and file data.
; After the entire file has been uploaded, an asterisk (*) is sent to indicate
; the end of the file. This command is used in conjunction with the DOWNLOAD command.
UPLOAD   LDA  #$0F     ;Send CR,LF, "Upload " to terminal
         JSR  PROMPT
         LDA  #$11     ;Send "address: " to terminal
         JSR  PROMPT
         LDA  #$04     ;Request upload start address input from terminal
         JSR  HEXIN
         TXA           ;GOTO XITUPLD IF no digits were entered
         BEQ  XITUPLD
         LDA  INDEX    ; ELSE, save start address on STACK
         PHA
         LDA  INDEXH
         PHA
         JSR  CROUT    ;Send CR,LF to terminal
         LDA  #$12     ;Send "Length: " to terminal
         JSR  PROMPT
         LDA  #$04     ;Request upload length input from terminal
         JSR  HEXIN
         TXA           ;GOTO DOUPLD IF any digits were entered
         BNE  DOUPLD
         PLA           ; ELSE, pull start address from STACK, discard
         PLA
XITUPLD  RTS           ;Done UPLOAD command, RETURN
DOUPLD   PLA           ;Restore start address from STACK
         TAX           ; High byte
         PLA
         TAY           ; Low byte
         CLC
         ADC  INDEX    ;Add start address to length: find end address 
         STA  TEMP2
         TXA
         ADC  INDEXH
         STA  TEMP2H   ;TEMP2 = end address pointer
         TXA
         STA  INDEXH   ;INDEX = start address pointer
         TYA
         STA  INDEX
UREADY   LDA  #$13     ;Send CR,LF, "10 Seconds" to terminal
         JSR  PROMPT
         LDX  #$38     ;Perform a 10 second delay
         JSR  SET      ; ($38 for MPU clock frequency = 1.8432 MHz)
UNXTLIN  JSR  CROUT    ;Send CR,LF to terminal
         LDX  #$00     ;Initialize line byte counter
         JSR  DOLLAR   ;Send "$" to terminal
         JSR  PRINDX   ;Display current address pointer
         JSR  SPC      ;Send [SPACE] to terminal
UNXTBYT  LDY  #$00
         LDA  (INDEX),Y
         STX  IDX      ;Save line byte counter
         JSR  BN2ASC   ;Convert byte to 2 ASCII HEX digits
         JSR  COUT     ;Send digits to terminal
         TYA
         JSR  COUT
         JSR  SPC      ;Send [SPACE] to terminal
         LDX  IDX      ;Restore line byte counter
         INX           ;Increment line byte counter
         JSR  INCINDEX ;Increment address pointer
         LDA  INDEXH   ;GOTO UCHEKEY IF INDEX <> TEMP2:
         CMP  TEMP2H   ;  (current address <> end address)
         BNE  UCHEKEY
         LDA  INDEX
         CMP  TEMP2
         BNE  UCHEKEY
DUNUPLD  LDA  #$2A     ; ELSE, send "*" to terminal: indicate end of file
         JSR  COUT
         RTS           ;Done UPLOAD command, RETURN
UCHEKEY  LDA  INCNT    ;GOTO KEYHIT IF a key was struck
         CMP  OUTCNT
         BNE  KEYHIT
         CPX  #$10     ; ELSE,
         BNE  UNXTBYT  ;  LOOP back to UNXTBYT IF line byte counter <> $10: continue current line
         JMP  UNXTLIN  ;   ELSE, LOOP back to UNXTLIN: start a new line
KEYHIT   STA  OUTCNT   ;Remove keystroke from keystroke input buffer
         JMP  DUNUPLD  ;GOTO DUNUPLD: abort UPLOAD command because a key was struck
;
;[V] VIEW TEXT command: Send bytes to terminal beginning at a specified address.
; Address is incremented then the process continues until a value of $00 is encountered  
VIEWTXT  JSR  COUT     ;Send "V" (command keystroke) to terminal
         JSR  SPC      ;Send [SPACE] to terminal
         LDA  #$04     ;Request starting view memory address input from terminal
         JSR  HEXIN    ; result in INDEX,INDEXH
VQUERY   JSR  CROUT    ;Send CR,LF to terminal
PROMPT2  LDY  #$00     ;Read from current view memory address
         LDA  (INDEX),Y
         BEQ  VIEWEXIT ;GOTO VIEWEXIT IF byte read = 0
         JSR  COUT     ; ELSE, send character/byte read to terminal
         JSR  INCINDEX ;Increment view memory address pointer
         JMP  PROMPT2  ;LOOP back to PROMPT2
VIEWEXIT RTS           ;Done VIEWTXT command, RETURN
;
;[W] WATCH command: Continuously read then display contents of a specified address, loop until keystroke 
WATCH    JSR  SETUP    ;Request HEX address input from terminal
WATCHL   JSR  READ     ;Read specified address
         JSR  PRBYTE   ;Display HEX value read
         JSR  DELAY2   ;Do a medium (~ 0.2 second) delay
         LDA  #$08     ;Send 2 [BACKSPACE] to terminal
         JSR  COUT2
         CLI           ;Enable IRQ service
         LDA  INCNT    ;LOOP back to WATCHL IF INCNT = OUTCNT: no key was struck
         CMP  OUTCNT
         BEQ  WATCHL   
         SEI           ; ELSE, Disable IRQ service
         INC  OUTCNT   ;Remove last keystroke from buffer
         RTS           ;Done WATCH command, RETURN
;
;[X] X-REGISTER command: Display then change X-reg preset/result
XRG      LDA  #$0A     ;Send "Xreg:$" to terminal
         JSR  PROMPT
         LDA  XREG     ;Read X-REGISTER preset/result
         JSR  CHREG    ;Display preset/result, request HEX byte input from terminal
         LDX  SCNT     ;GOTO NCXREG IF no digits were entered
         BEQ  NCXREG
         STA  XREG     ; ELSE, write to X-REGISTER preset/result
NCXREG   RTS           ;Done XRG command, RETURN
;
;[Y] Y-REGISTER command: Display then change Y-reg preset/result
YRG      LDA  #$0E     ;Send "Yreg:$" to terminal
         JSR  PROMPT
         LDA  YREG     ;Read Y-REGISTER preset/result
         JSR  CHREG    ;Display preset/result, request HEX byte input from terminal
         LDX  SCNT     ;GOTO NCYREG IF no digits were entered
         BEQ  NCYREG
         STA  YREG     ; ELSE, write to Y-REGISTER preset/result
NCYREG   RTS           ;Done YRG command, RETURN
;
;[Z] command: ZEDIT TEXT EDITOR enter/edit text beginning at a specified address
EDITOR   JSR  COUT     ;Send "Z" (command keystroke) to terminal
         JSR  SPC      ;Send [SPACE] to terminal
         LDA  #$04     ;Request starting edit memory address input from terminal
         JSR  HEXIN
         CPX  #$00     ;GOTO EDVWXIT IF no address was entered 
         BEQ  EDVWXIT
         JSR  CROUT    ;Send CR,LF to terminal
EDJMP1   LDA  INDEX    ; ELSE, save current edit line address (BACKSPACE process uses this address)  
         STA  TEMP3
         LDA  INDEXH
         STA  TEMP3H
EDJMP2   JSR  CHIN     ;Request a keystroke from terminal
         CMP  #$1B     ;GOTO EDITDUN IF keystroke = [ESCAPE]
         BEQ  EDITDUN
         CMP  #$0D     ; ELSE, GOTO ENOTRET IF keystroke <> [RETURN]
         BNE  ENOTRET
         LDY  #$00     ; ELSE, store [RETURN] in indexed edit memory 
         STA  (INDEX),Y
;STORLF (called below) is a patch subroutine. It follows the QUERY command text string array.
         JSR  STORLF   ;Send [RETURN] and [LINEFEED] to terminal then store [LINEFEED] in indexed edit memory
         JSR  INCINDEX ;Increment edit memory address pointer
         JMP  EDJMP1   ;LOOP back to EDJMP1 
ENOTRET  CMP  #$08     ;GOTO EDBKSPC IF keystroke = [BACKSPACE]
         BEQ  EDBKSPC
         LDY  #$00     ; ELSE, store keystroke in indexed edit memory
         STA  (INDEX),Y
         JSR  COUT     ;Send keystroke to terminal
         JSR  INCINDEX ;Increment edit memory address pointer
         JMP  EDJMP2   ;LOOP back to EDJMP2
EDBKSPC  LDA  INDEX    ;GOTO EDDOBKS IF INDEX,INDEXH > TEMP3,TEMP3H:
         CMP  TEMP3    ;  Do not allow BACKSPACE if already at the beginning
         BNE  EDDOBKS  ;  of the current edit line address
         LDA  INDEXH
         CMP  TEMP3H
         BNE  EDDOBKS
         JSR  BEEP     ; ELSE, send [BELL] to terminal
         JMP  EDJMP2   ;LOOP back to EDJMP2
EDDOBKS  JSR  BSOUT    ;Send [BACKSPACE] to terminal
         JSR  DECINDEX ;Decrement edit memory address pointer
         JMP  EDJMP2   ;LOOP back to EDJMP2
EDITDUN  JSR  CR2      ;Send 2 CR,LF to terminal
         JSR  DOLLAR   ;Send "$" to terminal
         JSR  PRINDX   ;Display current edit memory address
EDVWXIT  RTS           ;Done EDITOR command, RETURN
;
;[.] SWAIT command: Set delay duration then do delay
SWAIT    JSR  COUT     ;Send command keystroke to terminal
         JSR  HEXIN2   ;Request HEX byte input from terminal
         TAX           ;Save entered value in X-REGISTER
         LDA  SCNT     ;GOTO TIME IF no digits were entered
         BEQ  TIME
         JSR  SET      ; ELSE, Write entered value to SETIM variable and do delay 
EXPIRE   JMP  CROUT    ;Send CR,LF to terminal then done SWAIT or TIME command, RETURN
;[,] TIME command: Do delay 
TIME     JSR  TIMER    ;Do delay
         JMP  EXPIRE   ;Done TIME command, GOTO EXPIRE
;
;[(] INIMACRO command: Initialize keystroke input buffer: Fill buffer with $00,
; initialize written-to and read-from pointers to start of buffer. This erases
; all previous monitor command keystrokes from the keystroke input buffer in
; preparation for entering a monitor command macro.
INIMACRO LDA  #$02     ;Make memory fill start address high byte = $02
         STA  TEMP2H
         LDA  #$01     ;Make memory fill length high byte = $01
         STA  INDEXH
         LDA  #$00     ;Initialize to $00 the following:
         STA  TEMP2    ;  memory fill start address low byte
         STA  INDEX    ;  memory fill length low byte
         STA  INCNT    ;  buffer written-to pointer
         STA  OUTCNT   ;  buffer read-from pointer
         JMP  USERFILL ;Perform buffer fill then done INIMACRO command, RETURN
;
;[)] RUNMACRO command: Run monitor command macro. This will indicate that there
; are 256 keystrokes in the keystroke input buffer. The monitor will process these
; as if they were received from the terminal (typed-in by the user). Because the
; last keystroke stored in the keystroke buffer was ")", this will loop continuously.
; Use the [BREAK] key (or mulitple [RETURN]) to exit loop  
RUNMACRO LDA  #$00     ;Make keystroke buffer written-to pointer = $00
         STA  OUTCNT
         LDA  #$FF     ;Make keystroke buffer read-from pointer = $FF
         STA  INCNT
RET      RTS           ;Done RUNMACRO command, RETURN
;
;[CNTL-D] DOWNLOAD command: Receive a formatted ASCII HEX file from terminal,
; convert file from ASCII HEX to binary, store binary values in memory.
; The first character in the file MUST be a dollar sign ($), followed by FOUR
; ASCII HEX digits. These 4 digits represent the address at which to begin
; storing the downloaded/converted file data. What follows is the ASCII HEX
; representations of the binary file data, each byte being represented by TWO
; ASCII HEX digits (leading zeros MUST be included). IF a dollar sign ($) is
; again received, a new FOUR digit ASCII HEX address is expected, followed by
; more ASCII HEX file data.
; The last character in the file MUST be an asterisk (*). This indicates
; the end of the file and terminates the DOWNLOAD command.
; The ONLY printable ASCII characters allowed are: $*0123456789ABCDEF
; (note only upper-case ABCDEF)
; The ONLY ASCII control codes allowed are: [SPACE][RETURN][LINEFEED]
; All other ASCII input may cause errors. 
; The monitor [U] UPLOAD command produces formatted ASCII HEX file output to terminal
DOWNLOAD LDA  #$09     ;Send "Download:" to terminal   
         JSR  PROMPT
         LDA  #$E0     ;Point to $E0xx (ROM area) in case of garbage input:
         STA  INDEXH   ; write $E0 to download destination address pointer high byte
DLOOP    JSR  CHIN     ;Request a keystroke from terminal
         CMP  #$20     ; LOOP back to DLOOP IF character = [SPACE]: ignore character
         BEQ  DLOOP
         CMP  #$0D     ; ELSE, LOOP back to DLOOP IF character = [RETURN]: ignore character
         BEQ  DLOOP
         CMP  #$0A     ; ELSE, LOOP back to DLOOP IF character = [LINEFEED]: ignore character
         BEQ  DLOOP
         CMP  #$2A     ; ELSE, GOTO DBR1 IF chatacter <> "*": End Of File
         BNE  DBR1
         JMP  NMON     ; ELSE, done DOWNLOAD, GOTO NMON
; 
DBR1     CMP  #$24     ;GOTO DNEWADR IF character = "$"
         BEQ  DNEWADR
         JSR  DSUB1    ; ELSE, this is a high digit, go get low digit then convert ASCII HEX digits to binary
         LDY  #$00     ;Store byte at address pointed to by destination address pointer
         STA  (INDEX),Y
         JSR  INCINDEX ;Increment destination address pointer
         JMP  DLOOP    ;LOOP back to DLOOP
;
DNEWADR  JSR  DSUB2    ;Request 2 ASCII HEX digits from terminal then convert to binary 
         STA  INDEXH   ;Write value to destination address pointer high byte
         JSR  DSUB2    ;Request 2 ASCII HEX digits from terminal then convert to binary 
         STA  INDEX    ;Write value to destination address pointer low byte
         JMP  DLOOP    ;LOOP back to DLOOP
;
DSUB2    JSR  CHIN     ;Request a keystroke from terminal, result in ACCUMULATOR
DSUB1    PHA           ;Save ACCUMULATOR on STACK: ASCII HEX high digit of a byte  
         JSR  CHIN     ;Request a keystroke from terminal: ASCII HEX low digit
         TAY           ;Copy low digit to Y REGISTER
         PLA           ;Pull ACCUMULATOR from STACK: high digit
         JSR  ASC2BN   ;Convert high/low ASCII HEX digits to a binary value, result in ACCUMULATOR
         RTS           ;Done DSUB1 or DSUB2 subroutine, RETURN
;
;[CNTL-L] LISTER command: call disassembler
LISTER   LDA  #$FE     ;Point to assembler/disassembler prompt strings
         STA  PROHI
         JSR  LIST     ;Call disassembler
         LDA  #$FF     ;Point to monitor prompt strings
         STA  PROLO
         RTS           ;Done LISTER command, RETURN
;
;[CNTL-U] USER command: call user program at $0400 as a subroutine
USER     LDA  #$00     ;Make command address pointer = $0400  
         STA  COMLO
         LDA  #$04
         STA  COMHI
         JMP  CNTLUGO  ;GOTO CNTLUGO execute user code as a monitor command
                       ; (CNTLUGO is part of the [G] GO command listed above)
;
;[CNTL-W] WIPE command: clear (write: $00) RAM memory from $0000 through $7FFF then coldstart SyMon
WIPE     LDA  #$15     ;Send "Wipe RAM?" to terminal
         JSR  PROMPT
         LDA  #$FE     ;Point to prompt strings located at $FE00
         STA  PROHI
         LDA  #$01     ;Send CR,LF,"ESC key exits, any other to proceed" to terminal
         JSR  PROMPT
         JSR  CHIN     ;Request a keystroke from terminal
         CMP  #$1B     ;GOTO DOWIPE IF keystroke <> [ESC]
         BNE  DOWIPE
         RTS           ; ELSE, abort WIPE command, RETURN  
DOWIPE   LDA  #$02     ;Initialize temporary address pointer to $0002
         STA  $00
         LDA  #$00
         STA  $01
         TAX           ;Make index offset, ACCUMULATOR both = $00
WIPELOOP STA  ($00,X)  ;Write $00 to current address
         INC  $00      ;Increment address pointer
         BNE  WIPELOOP
         INC  $01
         BPL  WIPELOOP ;LOOP back to WIPELOOP IF address pointer < $8000
         STA  $01      ; ELSE, clear address pointer
         JMP  COLDSTRT ;Done WIPE command, GOTO COLDSTART
; 
;
;*********************************
;* COLDSTART: Power-up OR        *
;* pushbutton RESET vectors here *
;*********************************
;
COLDSTRT CLD           ;Put 6502 in binary arithmatic mode
         LDX  #$FF     ;Initialize STACK POINTER
         TXS
         LDA  #$1F     ;Initialize serial port (terminal I/O) 6551/65c51 ACIA
         STA  SIOCON   ; (19.2K BAUD,no parity,8 data bits,1 stop bit,
         LDA  #$09     ;  receiver IRQ output enabled)
         STA  SIOCOM
         LDA  #$FF     ;Initialize system variables as follows:
         STA  DELLO    ; delay time low byte
         STA  DELHI    ;  high byte
         STA  PROHI    ; prompt string buffer address pointer high byte
         LDA  #$00
         STA  PROLO    ; prompt string buffer address pointer low byte
         STA  ACCUM    ; ACCUMULATOR preset/result value
         STA  XREG     ; X-REGISTER preset/result value
         STA  YREG     ; Y-REGISTER preset/result value
         STA  PREG     ; PROCESSOR STATUS REGISTER preset/result value
         STA  OUTCNT   ; keystroke buffer 'read from' counter
         STA  INCNT    ; keystroke buffer 'written to' counter
         LDA  #$7F
         STA  SREG     ; USER program/application STACK POINTER preset/result value
                       ;<-FOR OPTIONAL LCD MODULE SUPPORT:*************************************************************
                       ; (See "commented-out" LCD interface routines located toward the end of this file) 
WARMST   NOP           ;<----------REPLACE THESE NOPS WITH
         NOP           ;<----------'JSR LCDPRMT' FOR LCD MODULE
         NOP           ;<----------SUPPORT (16 CH. BY 2 LINE MINIMUM)
         LDX  #$01     ;Set delay time
         JSR  SET      ; do short delay
         JSR  CR2      ;Send 2 CR,LF to terminal
;Send BIOS logon messages to terminal
         LDA  #$03     ;Send "S/O/S BIOS/monitor (c)1990 B.Phelps" to terminal
         JSR  PROMPT
         JSR  CROUT    ;Send CR,LF to terminal
         LDA  #$01     ;Send "Version mm.dd.yy" to terminal
         JSR  PROMPT
         JSR  CROUT    ;Send CR,LF to terminal
         LDA  #$02     ;Send "[ctrl-a] runs sub-assembler" to terminal
         JSR  PROMPT
         JSR  CR2      ;Send 2 CR,LF to terminal
         LDA  #$0B     ;Send "SyMon III" to terminal
         JSR  PROMPT
;
;
;********************** 
;* SyMon III monitor  *
;* command input loop *
;**********************
; 
MONITOR  LDA  #$00     ;Disable ASCII HEX/DEC digit filter
         STA  LOKOUT   ; (RDLINE subroutine uses this)
         JSR  CR2      ;Send 2 CR,LF to terminal
         LDA  #$0C     ;Send "monitor:" to terminal
         JSR  PROMPT
         JSR  BEEP     ;Send ASCII [BELL] to terminal
NMON     LDX  #$FF     ;Initialize STACK POINTER
         TXS
         STX  PROHI    ;Restore monitor prompt string buffer address pointer
         LDA  #$00     ; in case a monitor command, user or an application changed it
         STA  PROLO
         JSR  CR2      ;Send 2 CR,LF to terminal
         LDA  #$4D     ;Send "M" to terminal
         JSR  COUT
         LDA  #$2D     ;Send "-" to terminal
         JSR  COUT
CMON     JSR  RDCHAR   ;Wait for keystroke: RDCHAR converts lower-case Alpha. to upper-case  
         PHA           ;Save keystroke to STACK
         ASL  A        ;Multiply keystroke value by 2
         TAX           ;Get monitor command processor address from table MONTAB
         LDA  MONTAB,X ; using multiplied keystroke value as the index
         STA  COMLO    ;Store selected command processor address low byte
         INX
         LDA  MONTAB,X
         STA  COMHI    ;Store selected command processor address hi byte
         PLA           ;Restore keystroke from STACK: some command processors send keystroke to terminal
         JSR  DOCOM    ;Call selected monitor command processor as a subroutine
         JMP  NMON     ;LOOP back to NMON: command has been processed, go wait for next command 
DOCOM    JMP  (COMLO)  ;Go process command then RETURN
;
; 
;******************************
;* Monitor command jump table *
;******************************
;
;List of monitor command processor addresses. These are indexed by ASCII keystroke values
; which have been filtered (lower case Alpha. converted to upper case) then multiplied by 2
;        command:     keystroke: Keystroke value:  Monitor command function: 
MONTAB   .DW  RET      ;[BREAK]              $00  ([BREAK] keystroke is trapped by IRQ service routine)
         .DW  ASSEM    ;[CNTL-A]             $01  Call Sub-Assembler utility
         .DW  ERR      ;[CNTL-B]             $02
         .DW  ERR      ;[CNTL-C]             $03
         .DW  DOWNLOAD ;[CNTL-D]             $04  Download data/program file (MS HYPERTERM use: paste-to-host)
         .DW  ERR      ;[CNTL-E]             $05
         .DW  ERR      ;[CNTL-F]             $06
         .DW  ERR      ;[CNTL-G]             $07
         .DW  ERR      ;[CNTL-H],[BACKSPACE] $08
         .DW  ERR      ;[CNTL-I],[TAB]       $09
         .DW  ERR      ;[CNTL-J],[LINEFEED]  $0A
         .DW  ERR      ;[CNTL-K]             $0B
         .DW  LISTER   ;[CNTL-L]             $0C  Disassemble from specified address
         .DW  ERR      ;[CNTL-M],[RETURN]    $0D
         .DW  ERR      ;[CNTL-N]             $0E
         .DW  ERR      ;[CNTL-O]             $0F
         .DW  ERR      ;[CNTL-P]             $10
         .DW  ERR      ;[CNTL-Q]             $11
         .DW  COLDSTRT ;[CNTL-R]             $12  Restart same as power-up or RESET
         .DW  ERR      ;[CNTL-S]             $13
         .DW  ERR      ;[CNTL-T]             $14
         .DW  USER     ;[CNTL-U]             $15  Begin program code execution at address $0400    
         .DW  ERR      ;[CNTL-V]             $16
         .DW  WIPE     ;[CNTL-W]             $17  Clear memory ($0000-$7FFF) then restart same as power-up or COLDSTRT
         .DW  ERR      ;[CNTL-X]             $18
         .DW  ERR      ;[CNTL-Y]             $19
         .DW  ERR      ;[CNTL-Z]             $1A
         .DW  ERR      ;[CNTL-[],[ESCAPE]    $1B
         .DW  ERR      ;[CNTL-\]             $1C
         .DW  ERR      ;[CNTL-]]             $1D
         .DW  ERR      ;                     $1E
         .DW  ERR      ;                     $1F
         .DW  SPC2     ;[SPACE]              $20  Send [SPACE] to terminal (test for response, do nothing else)
         .DW  ERR      ; !                   $21
         .DW  ERR      ; "                   $22
         .DW  ERR      ; #                   $23
         .DW  ERR      ; $                   $24
         .DW  ERR      ; %                   $25
         .DW  ERR      ; &                   $26
         .DW  ERR      ; '                   $27
         .DW  INIMACRO ; (                   $28  Clear keystroke input buffer, Init buffer pointer to $00 (start)
         .DW  RUNMACRO ; )                   $29  Run keystroke macro from start of keystroke input buffer
         .DW  ERR      ; *                   $2A
         .DW  ERR      ; +                   $2B
         .DW  TIME     ; ,                   $2C  Perform delay
         .DW  ERR      ; -                   $2D
         .DW  SWAIT    ; .                   $2E  Set delay period, perform delay
         .DW  ERR      ; /                   $2F
         .DW  ERR      ; 0                   $30
         .DW  ERR      ; 1                   $31
         .DW  ERR      ; 2                   $32
         .DW  ERR      ; 3                   $33
         .DW  ERR      ; 4                   $34
         .DW  ERR      ; 5                   $35
         .DW  ERR      ; 6                   $36
         .DW  ERR      ; 7                   $37
         .DW  ERR      ; 8                   $38
         .DW  ERR      ; 9                   $39
         .DW  ERR      ; :                   $3A
         .DW  ERR      ; ;                   $3B
         .DW  ERR      ; <                   $3C
         .DW  ERR      ; =                   $3D
         .DW  ERR      ; >                   $3E
         .DW  ERR      ; ?                   $3F
         .DW  ERR      ; @                   $40
         .DW  ARG      ; A                   $41  Examine/change ACCUMULATOR preset/result
         .DW  BYTE     ; B                   $42  Examine ACCUMULATOR preset/result
         .DW  CHANGE   ; C                   $43  Examine/change a memory location's contents
         .DW  MDUMP    ; D                   $44  HEX dump from specified memory address
         .DW  EXAMINE  ; E                   $45  Examine a memory location's contents
         .DW  MFILL    ; F                   $46  Fill a specified memory range with a specified value
         .DW  GO       ; G                   $47  Begin program code execution at a specified address
         .DW  HEXTODEC ; H                   $48  Convert an entered HEX value to a BCD value, display result
         .DW  INPUT    ; I                   $49  Write next keystroke to ACCUMULATOR preset/result
         .DW  DECTOHEX ; J                   $4A  Convert an entered decimal value to a HEX value, display result
         .DW  SRCHBYT  ; K                   $4B  Search memory for a specified byte string
         .DW  SRCHTXT  ; L                   $4C  Search memory for a specified text string
         .DW  MOVER    ; M                   $4D  Copy a specified memory range to a specified target address
         .DW  ERR      ; N                   $4E  (Reserved: NEXT/NOTHING command)
         .DW  OUTPUT   ; O                   $4F  Send value (verbatim) in ACCUMULATOR preset/result to terminal  
         .DW  PRG      ; P                   $50  Examine/change PROCESSOR STATUS REGISTER preset/result
         .DW  QUERY    ; Q                   $51  Display list of useful system subroutines
         .DW  PRSTAT   ; R                   $52  Display all preset/result contents
         .DW  SRG      ; S                   $53  Examine/change STACK POINTER preset/result
         .DW  ADUMP    ; T                   $54  ASCII text dump from specified memory address
         .DW  UPLOAD   ; U                   $55  Upload data/program file 
         .DW  VIEWTXT  ; V                   $56  Examine (in ASCII) the contents from a specified memory location
         .DW  WATCH    ; W                   $57  Monitor a specified memory location's contents until a key is struck
         .DW  XRG      ; X                   $58  Examine/change X-REGISTER preset/result
         .DW  YRG      ; Y                   $59  Examine/change Y-REGISTER preset/result
         .DW  EDITOR   ; Z                   $5A  Call text editor utility
         .DW  ERR      ; [                   $5B
         .DW  ERR      ; \                   $5C
         .DW  ERR      ; ]                   $5D
         .DW  ERR      ; ^                   $5E
         .DW  ERR      ; _                   $5F
         .DW  ERR      ; `                   $60
;
;
;*************************************
;* IRQ/BRK Interrupt service routine *
;*************************************
;
INTERUPT STA  AINTSAV  ;Save ACCUMULATOR
         STX  XINTSAV  ;Save X-REGISTER
         STY  YINTSAV  ;Save Y-REGISTER
         LDA  SIOSTAT  ;Read 6551 ACIA status register
         AND  #$88     ;Isolate bits 7: Interrupt has occured and 3: receive data register full 
         EOR  #$88     ;Invert state of both bits 
         BNE  BRKINSTR ;GOTO BRKINSTR IF bit 7 OR bit 3 = 1: no valid data in receive data register  
         LDA  SIODAT   ; ELSE, read 6551 ACIA receive data register
         BEQ  BREAKEY  ;GOTO BREAKEY IF received byte = $00
         LDX  INCNT    ; ELSE, Store keystroke in keystroke buffer address
         STA  KEYBUFF,X ;  indexed by INCNT: keystroke buffer input counter
         INC  INCNT    ;Increment keystroke buffer input counter
ENDIRQ   LDA  AINTSAV  ;Restore ACCUMULATOR
         LDX  XINTSAV  ;Restore X-REGISTER
         LDY  YINTSAV  ;Restore Y-REGISTER
         RTI           ;Done INTERUPT (IRQ) service, RETURN FROM INTERRUPT
;
BRKINSTR PLA           ;Read PROCESSOR STATUS REGISTER from STACK
         PHA
         AND  #$10     ;Isolate BREAK bit
         BEQ  ENDIRQ   ;GOTO ENDIRQ IF bit = 0
         LDA  AINTSAV  ; ELSE, restore ACCUMULATOR to pre-interrupt condition 
         STA  ACCUM    ;Save in ACCUMULATOR preset/result
         PLA           ;Pull PROCESSOR STATUS REGISTER from STACK
         STA  PREG     ;Save in PROCESSOR STATUS preset/result
         STX  XREG     ;Save X-REGISTER
         STY  YREG     ;Save Y-REGISTER
         TSX
         STX  SREG     ;Save STACK POINTER
         JSR  CROUT    ;Send CR,LF to terminal
         PLA           ;Pull RETURN address from STACK then save it in INDEX
         STA  INDEX    ; Low byte
         PLA
         STA  INDEXH   ;  High byte
         JSR  CROUT    ;Send CR,LF to terminal
         JSR  PRSTAT   ;Display contents of all preset/result memory locations
         JSR  CROUT    ;Send CR,LF to terminal
         JSR  DISLINE  ;Disassemble then display instruction at address pointed to by INDEX
         LDA  #$00     ;Clear all PROCESSOR STATUS REGISTER bits
         PHA
         PLP
BREAKEY  LDX  #$FF     ;Set STACK POINTER to $FF
         TXS
         LDA  #$7F     ;Set STACK POINTER preset/result to $7F
         STA  SREG
         LDA  INCNT    ;Remove keystrokes from keystroke input buffer
         STA  OUTCNT
         JMP  NMON     ;Done interrupt service process, re-enter monitor
;
;
;***********************
;* S/O/S Sub-Assembler *
;*        v3.6         *
;* (c)1990 By B.Phelps *
;***********************
;
;Sub-assembler main loop
;
ASSEM    JSR  CR2      ;Send 2 CR,LF to terminal
         LDA  #$FE     ;Point to prompt strings located at $FE00 (assembler strings)
         STA  PROHI
         LDA  #$12     ;Send "S/O/S sub-assembler v3.6" to terminal
         JSR  PROMPT
         JSR  CR2      ;Send 2 CR,LF to terminal
         JSR  REORIG   ;Request origin address input from terminal (working address)
NEWLIN   JSR  CROUT    ;Send CR,LF to terminal
NLIN     JSR  DOLLAR   ;Send "$" to terminal
         JSR  PRINDX   ;Display current working address
         LDA  #$2D     ;Send "-" to terminal
         JSR  COUT
         JSR  SPC2     ;Send 2 [SPACE] to terminal
REENTR   LDX  #$03     ;Initialize mnemonic keystroke counter
ENTER    JSR  RDCHAR   ;Request a keystroke, convert lower-case Alpha. to upper-case
         CMP  #$1B     ;GOTO ANOTESC IF keytroke <> [ESCAPE]
         BNE  ANOTESC
         JMP  NMON     ;Done ASSEM, GOTO NMON: go back to monitor
ANOTESC  CMP  #$20     ;GOTO NOTSPC IF keytroke <> [SPACE]
         BNE  NOTSPC
         JSR  SAVLST   ; ELSE, save current working address
         LDA  #$0D     ;Send [RETURN] to terminal
         JSR  COUT
         JSR  DISLINE  ;Disassemble byte(s) found at working address, calculate next working address 
         JMP  NLIN     ;LOOP back to NLIN 
NOTSPC   CMP  #$2E     ;GOTO DIRECTV IF keystroke = "."
         BEQ  DIRECTV
NEM      CMP  #$5B     ; ELSE, GOTO TOOHI IF keystroke = OR > ("Z"+1)
         BCS  TOOHI
         CMP  #$41     ; ELSE, GOTO NOTLESS IF keystroke = OR > "A"
         BCS  NOTLESS
TOOHI    JSR  BEEP     ; ELSE, send [BELL] to terminal
         JMP  ENTER    ;LOOP back to ENTER
NOTLESS  STA  INBUFF-1,X ;Store keystroke in INBUFF indexed by mnemonic keystroke counter
         JSR  COUT     ;Send keystroke to terminal
         DEX           ;Decrement mnemonic keystroke counter
         BNE  ENTER    ;LOOP back to ENTER IF mnemonic keystroke counter <> 0
         LDY  #$01     ; ELSE, initialize mnemonic counter
RDLOOP2  LDA  MTAB,X   ;GOTO SKIP3 IF indexed character in MTAB <> keystroke in INBUFF+2
         CMP  INBUFF+2
         BNE  SKIP3
         INX           ; ELSE, increment mnemonic table index
         LDA  MTAB,X   ;GOTO SKIP2 IF indexed character in MTAB <> keystroke in INBUFF+1
         CMP  INBUFF+1
         BNE  SKIP2
         INX           ; ELSE, increment mnemonic table index
         LDA  MTAB,X   ;GOTO SKIP1 IF indexed character in MTAB <> keystroke in INBUFF
         CMP  INBUFF
         BNE  SKIP1
         TYA           ; ELSE, multiply mnemonic counter by 2: mnemonic handler jump table index
         CLC
         ASL  A
         TAX
         LDA  JTAB,X   ;Read indexed mnemonic handler address from jump table,  
         STA  COMLO    ; store address at COMLO,COMHI
         LDA  JTAB+1,X
         STA  COMHI
         JSR  SAVLST   ;Save current working address
         JMP  (COMLO)  ;GOTO mnemonic handler
;Abort compare against current indexed mnemonic, point to next mnemonic in mnemonic table 
SKIP3    INX           ;Increment mnemonic table index
SKIP2    INX           ;Increment mnemonic table index
SKIP1    INX           ;Increment mnemonic table index
         INY           ;Increment mnemonic counter
         CPY  #$46     ;LOOP back to RDLOOP2 IF mnemonic counter <> $46 (end of mnemonic table)
         BNE  RDLOOP2
         JSR  BEEP     ; ELSE, send [BELL] to terminal: user entered an invalid mnemonic
         LDA  #$08     ;Send 3 [BACKSPACE] to terminal
         JSR  COUT3
         JMP  REENTR   ;LOOP back to REENTER
;Process directives
DIRECTV  JSR  COUT     ;Send last keystroke (".") to terminal
         JSR  RDCHAR   ;Request a keystroke, convert lower-case Alpha. to upper-case
         LDX  #$00     ;Initialize directive table index
DIRTST   CMP  DIRTAB,X ;GOTO DIROK IF keystroke = indexed directive from DIRTAB
         BEQ  DIROK
         INX           ; ELSE, increment directive table index
         CPX  #$0F     ;LOOP back to DIRTST IF directive table index <> $0F (end of directive table) 
         BNE  DIRTST
         JSR  BEEP     ; ELSE, send [BELL] to terminal: user entered an invalid directive
         JMP  NEWLIN   ;LOOP back to NEWLIN
DIROK    CLC           ;multiply directive table index by 2: directive handler jump table index
         TXA
         ASL  A
         TAX
         LDA  DJTAB,X  ;Read indexed directive handler address from jump table,
         STA  COMLO    ; store address at COMLO,COMHI
         LDA  DJTAB+1,X
         STA  COMHI
         JSR  DODIR    ;Call directive handler as a subroutine
         JMP  NEWLIN   ;LOOP back to NEWLIN
DODIR    JMP  (COMLO)  ;GOTO directive handler 
;
;Assembler directive handlers:
;
;[.A] AARG directive subroutine: Examine/change ACCUMULATOR preset/result
AARG     LDA  #$FF     ;Point prompter to monitor prompt strings
         STA  PROHI
         JSR  ARG      ;Examine/change ACCUMULATOR preset/result
APROMPT  LDA  #$FE     ;Point prompter to assembler prompt strings
         STA  PROHI
         RTS           ;Done AARG or APROMPT directive subroutine, RETURN 
;
;[.B] BYTE directive subroutine: Request HEX byte(s) input from terminal, store byte(s) in working memory.
; This loops until [RETURN] is struck when no HEX digits have been entered
ABYTE    JSR  SAVLST   ;Save current working address
         LDA  #$0F     ;Send "Byte: " to terminal
         JSR  PROMPT
BYLOOP   JSR  SPC2     ;Send 2 [SPACE] to terminal
         JSR  HEXIN2   ;Request 1 or 2 HEX digit (data byte) input from terminal
         LDX  SCNT     ;GOTO BYOK IF any digits were entered
         BNE  BYOK
         RTS           ; ELSE, done ABYTE directive, RETURN 
BYOK     LDX  #$00     ;Store entered byte at current working address
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address
         JMP  BYLOOP   ;LOOP back to BYLOOP
;
;[.G] AGO directive subroutine: execute program code beginning at last specified ORIGIN address
AGO      JSR  CROUT    ;Send CR/LF to terminal
         LDA  TEMP2    ;Copy origin address to monitor command address pointer 
         STA  COMLO
         LDA  TEMP2H
         STA  COMHI
         JMP  CNTLUGO  ;GOTO CNTLUGO execute program code beginning at last specified ORIGIN address
;
;[.I] KEYCONV directive subroutine: display HEX equivalent of a keystroke to terminal 
KEYCONV  JSR  CHIN     ;Request a keystroke from terminal
         PHA           ;Save keystroke on STACK
         JSR  PRASC    ;Send keystroke to terminal IF printable ASCII, ELSE send "."
         LDA  #$3D     ;Send "=" to terminal
         JSR  COUT
         JSR  DOLLAR   ;Send "$" to terminal
         PLA           ;Restore keystroke value from STACK
         JMP  PRBYTE   ;Send ASCII HEX representation of keystroke to terminal then done, RETURN 
;
;[.L] LIST directive subroutine: list (disassemble) 21 instructions
LIST     LDA  #$14     ;Send "List: " to terminal
         JSR  PROMPT
         JSR  SAVLST   ;Save current working address
         JSR  HEXIN4   ;Request 1 to 4 HEX digit address input from terminal
         JSR  CROUT    ;Send CR,LF to terminal
         LDA  SCNT     ;GOTO LSTNEW IF any digits were entered
         BNE  LSTNEW
         LDA  TEMP2    ; ELSE, make working address = last specified origin address: 
         STA  INDEX    ;  list from last specified origin address
         LDA  TEMP2H
         STA  INDEXH
LSTNEW   LDA  #$15     ;Initialize instruction count: number of instructions to list = 21 ($15)
         STA  TEMP
DISLOOP  JSR  DISLINE  ;List (disassemble) one instruction
         DEC  TEMP     ;Decrement instruction count
         BNE  DISLOOP  ;LOOP back to DISLOOP IF instruction count <> 0
MORLOOP  JSR  CHIN     ; ELSE, request a keystroke from terminal
         CMP  #$0D     ;GOTO MORDIS IF keystroke <> [RETURN]
         BNE  MORDIS
         LDA  IDX      ; ELSE, make working address = saved working address:
         STA  INDEX    ;  restore working address to pre-LIST directive condition
         LDA  IDY
         STA  INDEXH
         RTS           ;Done LIST directive, RETURN
MORDIS   CMP  #$20     ;LOOP back to LSTNEW IF keystroke <> [SPACE]
         BNE  LSTNEW
         JSR  DISLINE  ;List (disassemble) one instruction
         JMP  MORLOOP  ;LOOP back to MORLOOP
;
;[.O] REORIGIN directive subroutine: Request new working address input from terminal
REORIG   LDA  #$11     ;Send "Origin: " to terminal 
         JSR  PROMPT
         JSR  HEXIN4   ;Request 1 to 4 HEX digit address input from terminal: enter new working address
         LDA  INDEX    ;Make origin address = new working address
         STA  TEMP2
         LDA  INDEXH
         STA  TEMP2H
         JMP  SAVLST   ;Make last working address = new working address
;
;[.P] APRG directive subroutine: Examine/change PROCESSOR STATUS preset/result
APRG     LDA  #$FF     ;Point prompter to monitor prompt strings
         STA  PROHI
         JSR  PRG      ;Examine/change PROCESSOR STATUS preset/result
         JMP  APROMPT  ;GOTO APROMPT: point prompter to assembler prompt strings then done, RETURN
;
;[.Q] AQUERY directive subroutine: display the system subroutine list
AQUERY   LDA  INDEX    ;Save current working address on STACK
         PHA
         LDA  INDEXH
         PHA
         LDA  #$51     ;Read "Q" (monitor command handler QUERY sends this to terminal)
         JSR  QUERY    ;Display the system subroutine list
         JSR  CROUT    ;Send CR/LF to terminal
         PLA           ;Restore working address from STACK 
         STA  INDEXH
         PLA
         STA  INDEX
         RTS           ;Done AQUERY directive, RETURN
;
;[.R] APRSTAT directive subroutine: Examine all presets/results
APRSTAT  JSR  CROUT    ;Send CR/LF to terminal
         JMP  PRSTAT   ;Examine all presets/results then done, RETURN
;
;[.S] ASRG directive subroutine: Examine/change STACK POINTER preset/result
ASRG     LDA  #$FF     ;Point prompter to monitor prompt strings
         STA  PROHI
         JSR  SRG      ;Examine/change STACK POINTER preset/result
         JMP  APROMPT  ;GOTO APROMPT: point prompter to assembler prompt strings then done, RETURN
;
;[.T] TEXT directive subroutine: Request text input from terminal
TEXT     LDA  #$08     ;Send "TEXT: " to terminal
         JSR  PROMPT
         JMP  EDJMP1   ;GOTO EDJMP1: call text editor then done TEXT directive, RETURN
;
;[.U] RECODE directive subroutine: edit last entered or disassembled instruction
RECODE   LDA  IDX      ;Make working address = previous (saved) assembled/disassembled instruction address 
         STA  INDEX
         LDA  IDY
         STA  INDEXH
         LDA  #$13     ;Send "Recode" to terminal then RETURN: done RECODE directive
         JMP  PROMPT
;
;[.W] WORD directive subroutine: Request HEX word(s) input from terminal, store word(s) in working memory.
; This loops until [RETURN] is struck when no HEX digits have been entered. Words are stored low byte first
WORD     JSR  SAVLST   ;Save current working address
         LDA  #$10     ;Send "Word: " to terminal
         JSR  PROMPT
WDLOOP   JSR  SPC2     ;Send 2 [SPACE] to terminal
         LDA  INDEX    ;Save current working address on STACK
         PHA
         LDA  INDEXH
         PHA
         JSR  HEXIN4   ;Request 1 to 4 HEX digit (data word) input from terminal 
         LDX  SCNT     ;GOTO WDOK IF any digits were entered
         BNE  WDOK
         PLA           ; ELSE, restore working address from STACK 
         STA  INDEXH
         PLA
         STA  INDEX
         RTS           ;Done WORD directive, RETURN
WDOK     LDA  INDEX    ;Save entered data word in OPLO,OPHI
         STA  OPLO
         LDA  INDEXH
         STA  OPHI
         PLA           ;Restore working address from STACK 
         STA  INDEXH
         PLA
         STA  INDEX
         LDX  #$00     ;Initialize working memory index (always = 0)
         LDA  OPLO     ;Store low byte of data word in working memory
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address pointer
         LDA  OPHI     ;Store high byte of data word in working memory
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address pointer
         JMP  WDLOOP   ;LOOP back to WDLOOP
;
;[.X] AXRG directive subroutine: Examine/change X-REGISTER preset/result
AXRG     LDA  #$FF     ;Point prompter to monitor prompt strings
         STA  PROHI
         JSR  XRG      ;Examine/change X-REGISTER preset/result
         JMP  APROMPT  ;GOTO APROMPT: point prompter to assembler prompt strings then done, RETURN
;
;[.Y] AYRG directive subroutine: Examine/change Y-REGISTER preset/result
AYRG     LDA  #$FF     ;Point prompter to monitor prompt strings
         STA  PROHI
         JSR  YRG      ;Examine/change Y-REGISTER preset/result
         JMP  APROMPT  ;GOTO APROMPT: point prompter to assembler prompt strings then done, RETURN
;
;Assembler/disassembler subroutines:
;DISLINE subroutine: disassemble (list) 1 instruction from working address
DISLINE  JSR  DOLLAR   ;Send "$" to terminal
         JSR  PRINDX   ;Display working address
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         LDY  #$00     ;Read opcode byte from working memory
         LDA  (INDEX),Y
         STA  SCNT     ;Save opcode byte (some handler subroutines need the opcode byte)
         JSR  PRBYTE   ;display opcode byte
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         TAX           ;Use opcode byte to index handler pointer table  
         LDA  HPTAB,X  ;Read indexed handler pointer
         TAX           ;Use handler pointer to index handler table
         LDA  HTAB,X   ;Read indexed handler address
         STA  COMLO    ; store handler address in COMLO,COMHI
         LDA  HTAB+1,X
         STA  COMHI
         JSR  DODIR    ;Call disassembler handler 
         JSR  CROUT    ;Send CR,LF to terminal
         JSR  INCNDX   ;Increment working address pointer
         RTS           ;Done DISLINE subroutine, RETURN
;
;CHEXIN2,AHEXIN2 subroutines: request byte value input from terminal,
; returns with result in OPLO
CHEXIN2  JSR  COUT     ;Send character in ACCUMULATOR to terminal 
AHEXIN2  TYA           ;Save Y-REGISTER on STACK
         PHA
H2LOOP   JSR  HEXIN2   ;Request 1 to 2 HEX digit input from terminal
         LDX  SCNT     ;GOTO H2OK IF any digits were entered
         BNE  H2OK
         JSR  ERRBS    ; ELSE, send [BELL],[BACKSPACE] to terminal
         JMP  H2LOOP   ;LOOP back to H2LOOP
H2OK     STA  OPLO     ;Store inputted byte in OPLO
         PLA           ;Restore Y-REGISTER from STACK
         TAY
         RTS           ;Done CHEXIN2 or AHEXIN2 subroutine, RETURN
; 
;SHEXIN4,AHEXIN4 subroutines: request word value input from terminal,
; returns with result in OPLO,OPHI
SHEXIN4  JSR  SPC2     ;Send 2 [SPACE] to terminal
AHEXIN4  TYA           ;Save Y-REGISTER on STACK
         PHA
         LDA  INDEX    ;Save working address on STACK
         PHA
         LDA  INDEXH
         PHA
H4LOOP   JSR  HEXIN4   ;Request 1 to 4 HEX digit input from terminal
         LDA  SCNT     ;GOTO H4OK IF any digits were entered
         BNE  H4OK
         JSR  ERRBS    ; ELSE, send [BELL],[BACKSPACE] to terminal
         JMP  H4LOOP   ;LOOP back to H4LOOP
H4OK     CMP  #$03     ;Set CARRY IF > 2 digits were entered, ELSE, clear it
         LDA  INDEX    ;Store inputted word in OPLO,OPHI
         STA  OPLO
         LDA  INDEXH
         STA  OPHI
         PLA           ;Restore working address pointer from STACK
         STA  INDEXH
         PLA
         STA  INDEX
         PLA           ;Restore Y-REGISTER from STACK
         TAY
         RTS           ;Done SHEXIN4 or AHEXIN4 subroutine, RETURN
;
;BITSEL subroutine: request bit number (0 to 7) input from terminal,
; multiply bit number value by $10
BITSEL   JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$38     ;GOTO NOTZ2S IF keystroke = OR > "8"
         BCS  NOTZ2S
         CMP  #$30     ; ELSE, GOTO Z2SOK IF keystroke = OR > "0"
         BCS  Z2SOK    ; ELSE,
NOTZ2S   JSR  BEEP     ;  send [BELL] to terminal
         JMP  BITSEL   ;LOOP back to BITSEL
Z2SOK    JSR  COUT     ;Send inputted digit ("0" to "7") to terminal
         ASL  A        ;Convert ASCII digit to HEX, multiply result times $10
         ASL  A
         ASL  A
         ASL  A
         RTS           ;Done BITSEL subroutine, RETURN
;
;ERRBS subroutine: send [BELL],[BACKSPACE] to terminal
ERRBS    JSR  BEEP     ;Send [BELL] to terminal
         JMP  BSOUT    ;Send [BACKSPACE] to terminal then done ERRBS subroutine, RETURN
;
;INCNDX subroutine: increment working address pointer then read from working address
;RDNDX subroutine: read from working address
INCNDX   JSR  INCINDEX ;Increment working address pointer
RDNDX    LDX  #$00     ;Read from working address
         LDA  (INDEX,X)
         RTS           ;Done INCNDX or RDNDX subroutine, RETURN
;
;PRMNEM subroutine: send 3 character mnemonic to terminal. Mnemonic string is
; indexed by opcode byte. Sends "***" if byte is not a valid opcode 
PRMNEM   LDX  SCNT     ;Retrieve opcode byte saved during DISLINE subroutine process
         LDA  MPTAB,X  ;Use opcode byte to index mnemonic pointer table
         TAX
         LDY  #$03     ;Initialize mnemonic character counter
PRML     LDA  DMTAB,X  ;Read indexed character from disassembler mnemonic table
         JSR  COUT     ;Send character to terminal
         INX           ;Increment mnemonic table index
         DEY           ;Decrement mnemonic character counter
         BNE  PRML     ;LOOP back to PRML IF counter <> 0
         JMP  SPC2     ; ELSE, send 2 [SPACE] to terminal then done PRMNEM subroutine, RETURN
;
;GETNXT subroutine: increment working address pointer, read from working address,
; display byte, send 2 [SPACE] to terminal. This displays operand byte(s)
GETNXT   JSR  INCNDX   ;Increment working address pointer, read from working address
         JSR  PRBYTE   ;Display byte
         JMP  SPC2     ;Send 2 [SPACE] to terminal then done GETNXT subroutine, RETURN
;
;TWOBYT subroutine: display operand byte then mnemonic of a two-byte instruction
TWOBYT   JSR  GETNXT   ;Read, display operand byte
         STA  OPLO     ;Save operand byte in OPLO
         LDA  #$06     ;Send 6 [SPACE] to terminal
         JSR  MSPC
         JMP  PRMNEM   ;Display mnemonic then done TWOBYT subroutine, RETURN
;
;TRIBYT subroutine: display operand bytes then mnemonic of a three-byte instruction
TRIBYT   JSR  GETNXT   ;Read, display operand low byte
         STA  OPLO     ;Save operand low byte in OPLO
         JSR  GETNXT   ;Read, display operand high byte
         STA  OPHI     ;Save operand high byte in OPLO
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JMP  PRMNEM   ;Display mnemonic then done TRIBYT subroutine, RETURN
;
;PR1 subroutine: display operand byte of a two-byte instruction (this follows mnemonic) 
PR1      JSR  DOLLAR   ;Send "$" to terminal
         LDA  OPLO     ;Display operand byte
         JMP  PRBYTE
;
;PR2 subroutine: display operand bytes of a three-byte instruction (this follows mnemonic)
PR2      JSR  DOLLAR   ;Send "$" to terminal
         LDA  OPHI     ;Display operand high byte
         JSR  PRBYTE
         LDA  OPLO     ;Display operand low byte
         JMP  PRBYTE   ; then done PR2 subroutine, RETURN
;
;SRMB2 subroutine: display 2 operand bytes, mnemonic, isolate bit selector from opcode 
;SRMB subroutine: display 1 operand byte, mnemonic, isolate bit selector from opcode 
SRMB2    JSR  RDNDX    ;Read from working address
         PHA           ;Save byte on STACK
         JSR  TRIBYT   ;Display operand bytes and mnemonic 
         JMP  SRM      ;GOTO SRM
SRMB     JSR  RDNDX    ;Read from working address
         PHA           ;Save byte on STACK
         JSR  TWOBYT   ;Display operand byte and mnemonic 
SRM      LDA  #$08     ;Send 2 [BACKSPACE] to terminal
         JSR  COUT2
         PLA           ;Restore byte from STACK
         LSR  A        ;Move high nybble to low nybble position,
         LSR  A        ; zero high nybble (bit selector)
         LSR  A
         LSR  A
         RTS           ;Done SRMB2 or SRMB subroutine, RETURN
;
;LSPRD subroutine: read indexed byte from opcode pointer table, then,
;SPCRD subroutine: send 2 [SPACE] to terminal, then,
;CLKRD subroutine: send "?" to terminal, request keystroke, erase "?" from terminal after keystroke
; returns with converted keystroke in ACCUMULATOR (LSPRD returns also with Y-REGISTER = opcode pointer)
LSPRD    LDA  OPTAB,Y  ;Read indexed byte from OPTAB (opcode pointer table)
         TAY           ;Transfer byte (opcode pointer) to Y-REGISTER
SPCRD    JSR  SPC2     ;Send 2 [SPACE] to terminal
CLKRD    LDA  #$3F     ;Send "?" to terminal
         JSR  COUT
         JSR  RDCHAR   ;Request keystroke from terminal, convert alpha. to upper-case
         PHA           ;Save keystroke on STACK
         JSR  BSOUT    ;Send [BACKSPACE] to terminal
         JSR  SPC      ;Send [SPACE] to terminal
         JSR  BSOUT    ;Send [BACKSPACE] to terminal
         PLA           ;Restore keystroke from STACK
         RTS           ;Done LSPRD,SPCRD or CLKRD subroutine, RETURN
;
;SAVLST subroutine: save a copy of the current working address
SAVLST   LDA  INDEX
         STA  IDX
         LDA  INDEXH
         STA  IDY
         RTS           ;Done SAVLST subroutine, RETURN
;
;COMX subroutine: send ",X" to terminal
COMX     LDA  #$2C     ;","
         JSR  COUT
         LDA  #$58     ;"X"
         JMP  COUT
;
;COMY subroutine: send ",Y" to terminal
COMY     LDA  #$2C     ;","
         JSR  COUT
         LDA  #$59     ;"Y"
         JMP  COUT
;
;RBR subroutine: send ")" to terminal
RBR      LDA  #$29     ;")"
         JMP  COUT
;
;LBR subroutine: send "(" to terminal
LBR      LDA  #$28     ;"("
         JMP  COUT
;
;MSPC subroutine: send [SPACE] to terminal (n) times where
; (n) = value in ACCUMULATOR
MSPC     JSR  SAVREGS  ;Save ACCUMULATOR, X,Y REGISTERS on STACK
         TAX           ;Initialize counter from ACCUMULATOR
         LDA  #$20     ;Read [SPACE]
SLOOP    JSR  COUT     ;Send it to terminal
         DEX           ;Decrement counter
         BNE  SLOOP    ;LOOP back to SLOOP IF counter <> 0
         JSR  RESREGS  ; ELSE, restore ACCUMULATOR, X,Y REGISTERS from STACK
         RTS           ;Done MSPC subroutine, RETURN
;
;BROFFSET subroutine: calculate branch offset from base address to target address.
; Use low byte of target address (entered value) IF it is out of branch range:
;  Subtract target address from base address, store 1's compliment of result, add 1 to stored result
BROFFSET SEC           ;Clear borrow flag
         LDA  COMLO    ;Read base address low byte
         SBC  OPLO     ;Subtract target address low byte
         EOR  #$FF     ;Store 1's complement of
         STA  TEMP3    ; result low byte
         LDA  COMHI    ;Read base address high byte
         SBC  OPHI     ;Subtract target address high byte (with borrow)
         EOR  #$FF     ;Store 1's complement of
         STA  TEMP3H   ; result high byte
         INC  TEMP3    ;Increment result low byte (MPU CARRY flag not affected here)
         BNE  WITCHWAY ;GOTO WITCHWAY IF result low byte <> 0: no carry produced    
         INC  TEMP3H   ; ELSE, increment result high byte: add carry to high byte
WITCHWAY BCC  BRFORWD  ;GOTO BRFORWD IF CARRY clear: base address < target address
                       ; ELSE,
; Determine if branch backward is within range:
;  TRAP EXCEPTION 1: branch FFhh to 00hh OR base address = target address 
         LDA  TEMP3H   ;GOTO EXCEPTN1 IF offset high byte = $00 
         BEQ  EXCEPTN1
         CMP  #$FF     ; ELSE, GOTO TOOFAR IF offset high byte = $FF
         BNE  TOOFAR
EXCEPTN2 LDA  TEMP3    ; ELSE, GOTO TOOFAR IF offset low byte < $80
         BPL  TOOFAR
BRANCHOK LDA  TEMP3    ;This holds the valid branch offset value
         STA  OPLO
         RTS           ;Done BROFFSET subroutine, RETURN
;Determine if branch forward is within range:
; TRAP EXCEPTION 2: branch 00hh to FFhh
BRFORWD  LDA  OPHI     ;GOTO NORMLFWD IF target address high byte <> $FF
         CMP  #$FF
         BNE  NORMLFWD
         LDA  COMHI    ; ELSE, GOTO EXCEPTN2 IF base address high byte = $00
         BEQ  EXCEPTN2
NORMLFWD LDA  TEMP3H   ; ELSE, GOTO TOOFAR IF offset high byte <> $00
         BNE  TOOFAR
EXCEPTN1 LDA  TEMP3    ; ELSE, GOTO BRANCHOK IF offset low byte < $80
         BPL  BRANCHOK
TOOFAR   LDA  #$15     ; ELSE, send "<- Offset = low byte" prompt to terminal,
         JMP  PROMPT   ;  then done BROFFSET subroutine, RETURN
;
;
;
;Disassembler handlers:
;
;SIN disassembler handler: single byte instructions: implied mode
; (note: ACCC handler calls this handler)   
SIN      LDA  #$0A     ;Send 10 [SPACE] to terminal
         JSR  MSPC
         JMP  PRMNEM   ;Display mnemonic then done SIN handler, RETURN
;
;ACCC disassembler handler: single byte instructions that modify ACCUMULATOR: implied mode 
ACCC     JSR  SIN      ;Send 10 [SPACE] to terminal then display mnemonic
         LDA  #$41     ;Send "A" to terminal
         JMP  COUT     ; then done ACCC handler, RETURN
;
;DABS disassembler handler: three byte instructions: absolute mode 
DABS     JSR  TRIBYT   ;Display operand bytes, then mnemonic
         JMP  PR2      ;Display operand bytes again then done DABS handler, RETURN
; 
;ZEROABS disassembler handler: two byte instructions: zero-page absolute
; (note: DZX,DZY handlers call this handler) 
ZEROABS  JSR  TWOBYT   ;Display operand byte, then mnemonic
         JMP  PR1      ;Display operand byte again then done ZEROABS handler, RETURN
;
;IME disassembler handler: two byte instructions: zero-page immediate mode
IME      JSR  TWOBYT   ;Display operand byte, then mnemonic
         LDA  #$23     ;Send "#" to terminal
         JSR  COUT
         JMP  PR1      ;Display operand byte again then done IME handler, RETURN
;
;DIND disassembler handler: three byte instruction: JMP (INDIRECT) 16 bit indirect mode
DIND     JSR  TRIBYT   ;Display operand bytes, then mnemonic
         JSR  LBR      ;Send "(" to terminal
         JSR  PR2      ;Display operand bytes again
         JMP  RBR      ;Send ")" to terminal then done DIND handler, RETURN
;
;ZPDIND disassembler handler: two or three byte instructions: indirect modes
ZPDIND   LDA  SCNT     ;Read saved opcode byte
         CMP  #$6C     ;GOTO ZPIND IF opcode byte <> $6C JMP(INDIRECT)
         BNE  ZPIND
         JMP  DIND     ; ELSE, GOTO DIND
; this is for a two byte instruction: zero page indirect mode 
ZPIND    JSR  TWOBYT   ;Display operand byte, then mnemonic
         JSR  LBR      ;Send "(" to terminal
         JSR  PR1      ;Display operand byte again
         JMP  RBR      ;Send ")" to terminal then done ZPDIND handler, RETURN
;
;DZX disassembler handler: two byte instructions: zero-page absolute indexed by X mode  
DZX      JSR  ZEROABS  ;Display operand byte, then mnemonic, then operand byte again
         JMP  COMX     ;Send ",X" to terminal then done DZX handler, RETURN
;
;DZY disassembler handler: two byte instructions: zero-page absolute indexed by Y mode
DZY      JSR  ZEROABS  ;Display operand byte, then mnemonic, then operand byte again
         JMP  COMY     ;Send ",Y" to terminal then done DZY handler, RETURN
;
;DAX disassembler handler: three byte instructions: absolute indexed by X mode
DAX      JSR  TRIBYT   ;Display operand bytes, then mnemonic
         JSR  PR2      ;Display operand bytes again
         JMP  COMX     ;Send ",X" to terminal then done DAX handler, RETURN
;
;DAY disassembler handler: three byte instructions: absolute indexed by Y mode
DAY      JSR  TRIBYT   ;Display operand bytes, then mnemonic
         JSR  PR2      ;Display operand bytes again
         JMP  COMY     ;Send ",Y" to terminal then done DAY handler, RETURN
;
;THX disassembler handler: two byte instructions: zero-page indirect pre-indexed by X mode  
THX      JSR  TWOBYT   ;Display operand byte, then mnemonic
         JSR  LBR      ;Send "(" to terminal
         JSR  PR1      ;Display operand byte again
         JSR  COMX     ;Send ",X" to terminal
         JMP  RBR      ;Send ")" to terminal then done THX handler, RETURN
;
;THY disassembler handler: two byte instructions: zero-page indirect post-indexed by Y mode 
THY      JSR  TWOBYT   ;Display operand byte, then mnemonic
         JSR  LBR      ;Send "(" to terminal
         JSR  PR1      ;Display operand byte again
         JSR  RBR      ;Send ")" to terminal
         JMP  COMY     ;Send ",Y" to terminal then done THY handler, RETURN
;
;INDABSX disassembler handler: three byte instruction: JMP (INDIRECT,X) 16 bit indirect
; pre-indexed by X mode
INDABSX  JSR  TRIBYT   ;Display operand bytes, then mnemonic
         JSR  LBR      ;Send "(" to terminal
         JSR  PR2      ;Display operand bytes again
         JSR  COMX     ;Send ",X" to terminal
         JMP  RBR      ;Send ")" to terminal then done INDABSX handler, RETURN
;
;DRMB disassembler handler: two byte instructions: zero page clear memory bits mode
; (note: DSMB,DBBR handlers call this handler at SRBIT) 
DRMB     JSR  SRMB     ;Display operand byte, mnemonic, isolate bit selector from opcode 
SRBIT    CLC           ;Convert bit selector value to an ASCII decimal digit
         ADC  #$30     ; (add "0" to bit selector value)
         JSR  COUT     ;Send digit to terminal
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JMP  PR1      ;Display operand byte again then done DRMB or SRBIT handler, RETURN
;
;DSMB disassembler handler: two byte instructions: zero page set memory bits mode 
DSMB     JSR  SRMB     ;Display operand byte, mnemonic, isolate bit selector from opcode 
         SEC           ;Clear M/S bit of selector low nybble: convert $08-$0F to $00-$07 
         SBC  #$08     ; (subtract 8 from bit selector value)
         JMP  SRBIT    ;GOTO SRBIT 
;
;DBBR disassembler handler: three byte instruction: branch on zero-page bit clear mode
DBBR     JSR  SRMB2    ;Display operand bytes, mnemonic, isolate bit selector from opcode 
SRBIT2   JSR  SRBIT    ;Convert and display bit selector digit, then display first operand byte again: ZP address
         LDA  OPHI     ;Move second operand to first operand position: 
         STA  OPLO     ; OPLO = branch offset
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JMP  BBREL    ;Display branch target address then done DBBR or DBBS handler, RETURN
;  
;DBBS disassembler handler: three byte instruction: branch on zero page bit set mode
DBBS     JSR  SRMB2    ;Display operand bytes, mnemonic, isolate bit selector from opcode 
         SEC           ;Clear M/S bit of selector low nybble: convert $08-$0F to $00-$07 
         SBC  #$08     ; (subtract 8 from bit selector value)
         JMP  SRBIT2   ;GOTO SRBIT2
;
;RELATIVE disassembler handler: two byte relative branch mode
;BBREL disassembler handler: three byte relative branch mode (called by DBBR,DBBS handlers)
; both calculate then display relative branch target address 
RELATIVE JSR  TWOBYT   ;Display operand byte, then mnemonic
BBREL    JSR  DOLLAR   ;Send "$" to terminal
         JSR  INCINDEX ;Increment working address: point to base (branch offset = 0) address
         LDA  OPLO     ;GOTO SUBTRACT IF offset > $7F
         BMI  SUBTRACT
         CLC           ; ELSE, add offset to base address: target address
         ADC  INDEX
         STA  COMLO
         LDA  INDEXH
         ADC  #$00
RELLOOP  JSR  PRBYTE   ;Send target address high byte to terminal
         LDA  COMLO    ;Send target address low byte to terminal
         JSR  PRBYTE
         JMP  DECINDEX ;Decrement (restore) working address then done RELATIVE handler, RETURN
SUBTRACT EOR  #$FF     ;Get 1's complement of offset
         STA  COMLO    ;Save result
         INC  COMLO    ;Increment result: value to subtract from base address
         SEC           ;Subtract adjusted offset from base address
         LDA  INDEX
         SBC  COMLO
         STA  COMLO
         LDA  INDEXH
         SBC  #$00
         JMP  RELLOOP  ;LOOP back to LELLOOP
;
;
;
;Assembler handlers:
;
;STOR assembler handler: write single byte instructions to working memory
;STOR2,CSTOR2,LSTOR2 assembler handlers: write 2 byte instructions to working memory
;STOR3,CSTOR3,LSTOR3 assembler handlers: write 3 byte instructions to working memory
; all of these jump back to the main assembler loop when done
CSTOR2   JSR  COUT     ;Send character in ACCUMULATOR to terminal 
LSTOR2   LDA  OCTAB,Y  ;Read indexed opcode from opcode table
STOR2    LDX  #$00     ;Write opcode to working address
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address pointer
         LDA  OPLO     ;Read operand byte
         JMP  STOR     ;GOTO STOR (write operand byte to working memory)      
CSTOR3   JSR  COUT     ;Send character in ACCUMULATOR to terminal 
LSTOR3   LDA  OCTAB,Y  ;Read indexed opcode from opcode table
STOR3    LDX  #$00     ;Write opcode to working address
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address pointer
         LDA  OPLO     ;Read operand low byte
         STA  (INDEX,X);Write operand low byte to working address
         JSR  INCNDX   ;Increment working address pointer
         LDA  OPHI     ;Read operand high byte
STOR     LDX  #$00)    ;Write operand high byte to working address
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address pointer
         JMP  NEWLIN   ;Done STOR assembler handler, GOTO NEWLIN
;
;LDACC assembler handler:
LDACC    JSR  LSPRD    ;Read opcode pointer and request opcode modifier keystroke
         CMP  #$23     ;GOTO NOTIMED IF modifier <> "#"
         BNE  NOTIMED
         JSR  CHEXIN2  ; ELSE, send modifier to terminal, request operand byte input from terminal
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
NOTIMED  INY           ;Increment opcode table index
STAIN    CMP  #$28     ;GOTO NOTIND IF modifier <> "("
         BNE  NOTIND
         JSR  COUT     ; ELSE, send modifier to terminal
         JSR  AHEXIN4  ;Request operand word input from terminal
         BCC  NOIND    ;GOTO NOIND IF < 3 digits were entered
         LDA  #$29     ; ELSE, send ")" to terminal
         JSR  COUT
         LDA  OCTAB,Y  ;Read indexed opcode from opcode table
         AND  #$0F     ;GOTO BFIXBR1 IF low digit of opcode = 2: not JMP ($nnnn) instruction
         CMP  #$02     ; This allows MMM ($nnnx), encodes as: MMM ($nn) 
         BEQ  BFIXBR1  ;  unless it is a JMP ($nnnx) instruction
         JMP  LSTOR3   ;   ELSE, write 3 byte instruction to working memory then GOTO NEWLIN
;
BFIXBR1  LDA  OCTAB,Y  ;Read indexed opcode byte from opcode table
         LDX  #$00     ;Write opcode byte to working memory 
         STA  (INDEX,X)
         JSR  INCNDX   ;Increment working address pointer
         LDA  OPLO     ;READ operand byte
         STA  (INDEX,X);Write operand byte to working address
         JSR  INCNDX   ;Increment working address pointer
         JMP  NEWLIN   ;Done LDACC handler, GOTO NEWLIN
;
NOIND    INY           ;Increment opcode table index
         JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$29     ;GOTO INDX IF keystroke <> ")"
         BNE  INDX
         JSR  COUT     ; ELSE, send keystroke to terminal
         JSR  COMY     ;Send ",Y" to terminal
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
INDX     INY           ;Increment opcode table index
         JSR  COMX     ;Send ",X" to terminal
         LDA  #$29     ;Read ")": this will be sent to terminal by CSTOR2
         JMP  CSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
NOTIND   INY           ;Add 3 to opcode table index
         INY
         INY
         JSR  AHEXIN4  ;Request operand word input from terminal
         BCS  LDAABS   ;GOTO LDAABS IF > 2 digits were entered
         JMP  LDAZX    ; ELSE, GOTO LDAZX
LDAABS   INY           ;Add 2 to opcode table index
         INY
         JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO NOTIXY IF keystroke <> ","
         BNE  NOTIXY
         JSR  COUT     ; ELSE, send keystroke to terminal
         JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$59     ;GOTO LDIX IF keystroke <> "Y"
         BNE  LDIX
         JMP  CSTOR3   ; ELSE, Write 3 byte instruction to working memory then GOTO NEWLIN 
;
LDIX     INY           ;Increment opcode table index
         LDA  #$58     ;Read "X": this will be sent to terminal by CSTOR3
         JMP  CSTOR3   ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
NOTIXY   INY           ;Add 2 to opcode table index
         INY
         JMP  LSTOR3   ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
BITS     JSR  LSPRD    ;Read opcode pointer and request opcode modifier keystroke
         CMP  #$23     ;GOTO NOTA IF keystroke <> "#"
         BNE  NOTA
         JSR  CHEXIN2  ; ELSE, send modifier to terminal, request operand byte input from terminal
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
SHRO     JSR  LSPRD    ;Read opcode pointer and request opcode modifier keystroke
         CMP  #$41     ;GOTO NOTA IF keystroke <> "A"
         BNE  NOTA
         JSR  COUT     ; ELSE, send keystroke to terminal
         LDA  OCTAB,Y  ;Read indexed opcode from opcode table
         JMP  STOR     ;Write 1 byte instruction to working memory then GOTO NEWLIN
;
NOTA     INY           ;Increment opcode table index
NEVERA   JSR  AHEXIN4  ;Request operand word input from terminal
         BCS  SHROAB   ;GOTO SHROAB IF > 2 digits were entered
LDAZX    JSR  CLKRD    ; ELSE, send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO SHROZP IF keystroke <> ","
         BNE  SHROZP
         JSR  COMX     ; ELSE, send ",X" to terminal
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
SHROZP   INY           ;Increment opcode table index
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
SHROAB   INY           ;Add 2 to opcode table index
         INY
         JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO SRAB IF keystroke <> ","
         BNE  SRAB
         JSR  COMX     ; ELSE, send ",X" to terminal
         JMP  LSTOR3   ;write 3 byte instruction to working memory then GOTO NEWLIN
;
SRAB     INY           ;Increment opcode table index
         JMP  LSTOR3   ;write 3 byte instruction to working memory then GOTO NEWLIN
;
LDXY     JSR  LSPRD    ;Read opcode pointer and request opcode modifier keystroke
         CMP  #$23     ;GOTO LDNIM IF keystroke <> "#"
         BNE  LDNIM
         JSR  CHEXIN2  ; ELSE, send modifier to terminal, request operand byte input from terminal
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
LDNIM    INY           ;Increment opcode table index
         JSR  AHEXIN4  ;Request operand word input from terminal
         BCS  LDAB     ;GOTO LDAB IF > 2 digits were entered
         JSR  CLKRD    ; ELSE, send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO LDZP IF keystroke <> ","
         BNE  LDZP
         JSR  COUT     ; ELSE, send keystroke to terminal
         LDA  OCTAB,Y  ;Read indexed opcode from opcode table
         INY           ;Increment opcode table index
         JMP  CSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
LDZP     INY           ;Add 2 to opcode table index
         INY
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
LDAB     INY           ;Add 3 to opcode table index
         INY
         INY
         JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO LDABS IF keystroke <> ","
         BNE  LDABS
         JSR  COUT     ; ELSE, send keystroke to terminal
         LDA  OCTAB,Y  ;Read indexed CHARACTER from opcode table: this is an ASCII "Y" 
         JSR  COUT     ;Send character to terminal
         INY           ;Increment opcode table index
         JMP  LSTOR3   ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
LDABS    INY           ;Add 2 to opcode table index
         INY
         JMP  LSTOR3   ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
STXY     LDA  OPTAB,Y  ;Read indexed byte from OPTAB (opcode pointer table)
         TAY
         JSR  SHEXIN4  ;Request word value input from terminal, returns with result in OPLO,OPHI
         BCS  STAB     ;GOTO STAB IF > 2 digits were entered
         JSR  CLKRD    ; ELSE, send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO STZP IF keystroke <> ","
         BNE  STZP
         JSR  COUT     ; ELSE, send keystroke to terminal
         LDA  OCTAB,Y  ;Read indexed CHARACTER from opcode table: this is an ASCII "X"
         JSR  COUT     ;Send character to terminal
         INY           ;Increment opcode table index
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
STZP     INY           ;Add 2 to opcode table index
         INY
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
STAB     INY           ;Add 3 to opcode table index
         INY
         INY
         JMP  LSTOR3   ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
TSRB     LDA  OPTAB,Y  ;Read indexed byte from OPTAB (opcode pointer table)
         TAY
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JMP  TSRBIN   ;GOTO TSRBIN
;
CPXY     JSR  LSPRD    ;Read opcode pointer and request opcode modifier keystroke
         CMP  #$23     ;GOTO XYNOIM IF keystroke <> "#"
         BNE  XYNOIM
         JSR  CHEXIN2  ; ELSE, send modifier to terminal, request operand byte input from terminal
         JMP  LSTOR2   ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
XYNOIM   INY           ;Increment opcode table index
TSRBIN   JSR  AHEXIN4  ;Request operand word input from terminal
         BCS  CPXYAB   ;GOTO CPXYAB IF > 2 digits were entered
         JMP  LSTOR2   ; ELSE, write 2 byte instruction to working memory then GOTO NEWLIN
;
CPXYAB   INY           ;Increment opcode table index
         JMP  LSTOR3   ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
JUMPS    JSR  SPCRD    ;Send 2 [SPACE] then "?" to terminal, request keystroke from terminal
         CMP  #$28     ;GOTO INDJ IF keystroke = "("
         BEQ  INDJ
         JSR  AHEXIN4  ; ELSE, request operand word input from terminal
         LDA  #$4C     ;Read JMP $nnnn JMP ABSOLUTE opcode
         JMP  STOR3    ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
INDJ     JSR  COUT     ;Send keystroke to terminal
         JSR  AHEXIN4  ;Request operand word input from terminal
         JSR  CLKRD    ;Send "?" to terminal, request keystroke from terminal
         CMP  #$2C     ;GOTO NOABINX IF keystroke <> ","
         BNE  NOABINX
         JSR  COMX     ;Send ",X" to terminal
         JSR  RBR      ;Send ")" to terminal
         LDA  #$7C     ;Read JMP ($nnnn,X) JMP INDIRECT PRE-INDEXED BY X opcode
         JMP  STOR3    ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
NOABINX  JSR  RBR      ;send ")" to terminal
         LDA  #$6C     ;Read JMP ($nnnn) JMP INDIRECT opcode
         JMP  STOR3    ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
JSUB     JSR  SHEXIN4  ;Request word value input from terminal, returns with result in OPLO,OPHI
         LDA  #$20     ;Read JSR $nnnn JSR ABSOLUTE opcode
         JMP  STOR3    ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
RMB      JSR  BITSEL   ;Request bit number (0 to 7) input from terminal
         JMP  BITPOST  ;GOTO BITPOST
SMB      JSR  BITSEL   ;Request bit number (0 to 7) input from terminal
         CLC           ;Add $80 to bit selector
         ADC  #$80
BITPOST  ORA  OPTAB,Y  ;Logical OR with the indexed opcode pointer table
         STA  OPHI     ;Save opcode table pointer
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JSR  AHEXIN2  ;Request operand byte input from terminal
         LDA  OPHI     ;Restore opcode table pointer
         JMP  STOR2    ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
STZ      LDA  OPTAB,Y  ;Read indexed byte from OPTAB (opcode pointer table)
         TAY           ;Make this the OCTAB index (opcode table index)
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JMP  NEVERA   ;GOTO NEVERA
;
REL      JSR  SHEXIN4   ;Request word value input from terminal, returns with result in OPLO,OPHI: target address
         CLC            ;Calculate base address: base address = working address + 2, 
         LDA  INDEX     ; store result in COMLO,COMHI
         ADC  #$02
         STA  COMLO
         LDA  INDEXH
         ADC  #$00
         STA  COMHI 
         JSR  BROFFSET ;Calculate branch offset from base address to target address
         LDA  OPTAB,Y  ;Read indexed byte from OPTAB (opcode pointer table)
         JMP  STOR2    ;Write 2 byte instruction to working memory then GOTO NEWLIN
;
IMP      LDA  OPTAB,Y  ;Read indexed byte from OPTAB (opcode pointer table)
         JMP  STOR     ;Write 1 byte instruction to working memory then GOTO NEWLIN
;
STACC    JSR  LSPRD    ;Read opcode pointer and request opcode modifier keystroke
         JMP  STAIN    ;GOTO STAIN
;
BBR      JSR  BITSEL   ;Request bit number (0 to 7) input from terminal
         JMP  BBRS     ;GOTO BBRS
BBS      JSR  BITSEL   ;Request bit number (0 to 7) input from terminal
         CLC           ;Add $80 to bit selector: convert 0-7 to 8-F
         ADC  #$80
BBRS     ORA  OPTAB,Y  ;Logical OR with the indexed opcode pointer table
         PHA           ;Save opcode table pointer on STACK
         JSR  SPC2     ;Send 2 [SPACE] to terminal
         JSR  AHEXIN2  ;Request operand byte input from terminal
         LDA  OPLO     ;Save operand low byte on STACK: zero-page address 
         PHA
         JSR  SHEXIN4  ;Request word value input from terminal, returns with result in OPLO,OPHI: target address
         CLC           ;Calculate base address: base address = working address + 3, 
         LDA  INDEX    ; store result in COMLO,COMHI 	
         ADC  #$03
         STA  COMLO
         LDA  INDEXH
         ADC  #$00
         STA  COMHI
         JSR  BROFFSET ;Calculate branch offset from base address to target address
         LDA  OPLO     ;Move branch offset to operand high byte position: branch offset
         STA  OPHI
         PLA           ;Restore operand low byte from STACK: zero-page address 
         STA  OPLO
         PLA           ;Restore opcode table pointer from STACK
         JMP  STOR3    ;Write 3 byte instruction to working memory then GOTO NEWLIN
;
;Assembler/disassembler tables:
;
;Disassembler mnemonic pointer table. This is indexed by the instruction OPCODE value.
; The byte elements in this table point to the proper MNEMONIC STRING for the OPCODE being disassembled: 
MPTAB    .DB $03,$A2,$00,$00,$C6,$A2,$6C,$B7,$24,$A2,$6C,$00,$C6,$A2,$6C,$BD
         .DB $5D,$A2,$A2,$00,$C3,$A2,$6C,$B7,$06,$A2,$84,$00,$C3,$A2,$6C,$BD
         .DB $69,$96,$00,$00,$78,$96,$72,$B7,$2A,$96,$72,$00,$78,$96,$72,$BD
         .DB $57,$96,$96,$00,$78,$96,$72,$B7,$33,$96,$81,$00,$78,$96,$72,$BD
         .DB $2D,$9C,$00,$00,$00,$9C,$6F,$B7,$21,$9C,$6F,$00,$66,$9C,$6F,$BD
         .DB $60,$9C,$9C,$00,$00,$9C,$6F,$B7,$0C,$9C,$AE,$00,$00,$9C,$6F,$BD
         .DB $30,$93,$00,$00,$C9,$93,$75,$B7,$27,$93,$75,$00,$66,$93,$75,$BD
         .DB $63,$93,$93,$00,$C9,$93,$75,$B7,$39,$93,$B4,$00,$66,$93,$75,$BD
         .DB $CC,$A8,$00,$00,$90,$A8,$8D,$BA,$15,$78,$45,$00,$90,$A8,$8D,$C0
         .DB $4E,$A8,$A8,$00,$90,$A8,$8D,$BA,$4B,$A8,$48,$00,$C9,$A8,$C9,$C0
         .DB $8A,$9F,$87,$00,$8A,$9F,$87,$BA,$3F,$9F,$3C,$00,$8A,$9F,$87,$C0
         .DB $51,$9F,$9F,$00,$8A,$9F,$87,$BA,$0F,$9F,$42,$00,$8A,$9F,$87,$C0
         .DB $7E,$99,$00,$00,$7E,$99,$81,$BA,$1B,$99,$12,$00,$7E,$99,$81,$C0
         .DB $5A,$99,$99,$00,$00,$99,$81,$BA,$09,$99,$AB,$00,$00,$99,$81,$C0
         .DB $7B,$A5,$00,$00,$7B,$A5,$84,$BA,$18,$A5,$1E,$00,$7B,$A5,$84,$C0
         .DB $54,$A5,$A5,$00,$00,$A5,$84,$BA,$36,$A5,$B1,$00,$00,$A5,$84,$C0
;
;Disassembler handler pointer table. This is indexed by the instruction OPCODE value.
; The byte elements in this table point to the proper HANDLER for the OPCODE being disassembled:
HPTAB    .DB $00,$0C,$00,$00,$06,$06,$06,$1C,$00,$04,$02,$00,$10,$10,$10,$20
         .DB $18,$0E,$16,$00,$06,$08,$08,$1C,$00,$14,$02,$00,$10,$12,$12,$20
         .DB $10,$0C,$00,$00,$06,$06,$06,$1C,$00,$04,$02,$00,$10,$10,$10,$20
         .DB $18,$0E,$16,$00,$08,$08,$08,$1C,$00,$14,$02,$00,$12,$12,$12,$20
         .DB $00,$0C,$00,$00,$00,$06,$06,$1C,$00,$04,$02,$00,$10,$10,$10,$20
         .DB $18,$0E,$16,$00,$00,$08,$08,$1C,$00,$14,$00,$00,$00,$12,$12,$20
         .DB $00,$0C,$00,$00,$06,$06,$06,$1C,$00,$04,$02,$00,$16,$10,$10,$20
         .DB $18,$0E,$16,$00,$08,$08,$08,$1C,$00,$14,$00,$00,$1A,$12,$12,$20
         .DB $18,$0C,$00,$00,$06,$06,$06,$1E,$00,$04,$00,$00,$10,$10,$10,$22
         .DB $18,$0E,$16,$00,$08,$08,$0A,$1E,$00,$14,$00,$00,$10,$12,$12,$22
         .DB $04,$0C,$04,$00,$06,$06,$06,$1E,$00,$04,$00,$00,$10,$10,$10,$22
         .DB $18,$0E,$16,$00,$08,$08,$0A,$1E,$00,$14,$00,$00,$12,$12,$14,$22
         .DB $04,$0C,$00,$00,$06,$06,$06,$1E,$00,$04,$00,$00,$10,$10,$10,$22
         .DB $18,$0E,$16,$00,$00,$08,$08,$1E,$00,$14,$00,$00,$00,$12,$12,$22
         .DB $04,$0C,$00,$00,$06,$06,$06,$1E,$00,$04,$00,$00,$10,$10,$10,$22
         .DB $18,$0E,$16,$00,$00,$08,$08,$1E,$00,$14,$00,$00,$00,$12,$12,$22
;
;Disassembler handler table:
;   Handler address:   Handler address index: (referenced in table HPTAB)
HTAB     .DW SIN           ;$00
         .DW ACCC          ;$02        
         .DW IME           ;$04
         .DW ZEROABS       ;$06
         .DW DZX           ;$08
         .DW DZY           ;$0A
         .DW THX           ;$0C
         .DW THY           ;$0E
         .DW DABS          ;$10
         .DW DAX           ;$12
         .DW DAY           ;$14
         .DW ZPDIND        ;$16
         .DW RELATIVE      ;$18
         .DW INDABSX       ;$1A
         .DW DRMB          ;$1C
         .DW DSMB          ;$1E
         .DW DBBR          ;$20
         .DW DBBS          ;$22
;
;DMTAB: disassembler mnemonic table,
;MTAB: assembler mnemonic table:
;     mnemonic string:
DMTAB    .DB "***"
MTAB     .DB "BRK"
         .DB "CLC"
         .DB "CLD"
         .DB "CLI"
         .DB "CLV"
         .DB "DEX"
         .DB "DEY"
         .DB "INX"
         .DB "INY"
         .DB "NOP"
         .DB "PHA"
         .DB "PHP"
         .DB "PLA"
         .DB "PLP"
         .DB "RTI"
         .DB "RTS"
         .DB "SEC"
         .DB "SED"
         .DB "SEI"
         .DB "TAX"
         .DB "TAY"
         .DB "TSX"
         .DB "TXA"
         .DB "TXS"
         .DB "TYA"
         .DB "BCC"
         .DB "BCS"
         .DB "BEQ"
         .DB "BMI"
         .DB "BNE"
         .DB "BPL"
         .DB "BVC"
         .DB "BVS"
         .DB "JMP"
         .DB "JSR"
         .DB "ASL"
         .DB "LSR"
         .DB "ROL"
         .DB "ROR"
         .DB "BIT"
         .DB "CPX"
         .DB "CPY"
         .DB "DEC"
         .DB "INC"
         .DB "LDX"
         .DB "LDY"
         .DB "STX"
         .DB "STY"
         .DB "ADC"
         .DB "AND"
         .DB "CMP"
         .DB "EOR"
         .DB "LDA"
         .DB "ORA"
         .DB "SBC"
         .DB "STA"
         .DB "PHX"
         .DB "PHY"
         .DB "PLX"
         .DB "PLY"
         .DB "RMB"
         .DB "SMB"
         .DB "BBR"
         .DB "BBS"
         .DB "TRB"
         .DB "TSB"
         .DB "STZ"
         .DB "BRA"
;
;Assembler handler jump table:
JTAB     .DW $FFFF
         .DW IMP,IMP,IMP,IMP,IMP,IMP,IMP,IMP
         .DW IMP,IMP,IMP,IMP,IMP,IMP,IMP,IMP
         .DW IMP,IMP,IMP,IMP,IMP,IMP,IMP,IMP
         .DW IMP,REL,REL,REL,REL,REL,REL,REL
         .DW REL,JUMPS,JSUB,SHRO,SHRO,SHRO,SHRO,BITS
         .DW CPXY,CPXY,SHRO,SHRO,LDXY,LDXY,STXY,STXY
         .DW LDACC,LDACC,LDACC,LDACC,LDACC,LDACC,LDACC,STACC
         .DW IMP,IMP,IMP,IMP,RMB,SMB,BBR,BBS
         .DW TSRB,TSRB,STZ,REL
;
;Assembler opcode pointer table:
OPTAB    .DB $FF,$00,$18,$D8,$58,$B8,$CA,$88,$E8,$C8,$EA,$48,$08,$68,$28,$40
         .DB $60,$38,$F8,$78,$AA,$A8,$BA,$8A,$9A,$98,$90,$B0,$F0,$30,$D0,$10
         .DB $50,$70,$FF,$FF,$10,$15,$1A,$1F,$81,$00,$03,$06,$0B,$24,$2B,$32
         .DB $36,$3A,$43,$4C,$55,$5E,$67,$70,$79,$DA,$5A,$FA,$7A,$07,$07,$0F
         .DB $0F,$8A,$8C,$86,$80
;
;Assembler opcode table:
OCTAB    .DB $E0,$E4,$EC,$C0,$C4,$CC,$3A,$D6,$C6,$DE,$CE,$1A,$F6,$E6,$FE,$EE
         .DB $0A,$16,$06,$1E,$0E,$4A,$56,$46,$5E,$4E,$2A,$36,$26,$3E,$2E,$6A
         .DB $76,$66,$7E,$6E,$A2,$59,$B6,$A6,$59,$BE,$AE,$A0,$58,$B4,$A4,$58
         .DB $BC,$AC,$59,$96,$86,$8E,$58,$94,$84,$8C,$69,$72,$71,$61,$75,$65
         .DB $79,$7D,$6D,$29,$32,$31,$21,$35,$25,$39,$3D,$2D,$C9,$D2,$D1,$C1
         .DB $D5,$C5,$D9,$DD,$CD,$49,$52,$51,$41,$55,$45,$59,$5D,$4D,$A9,$B2
         .DB $B1,$A1,$B5,$A5,$B9,$BD,$AD,$09,$12,$11,$01,$15,$05,$19,$1D,$0D
         .DB $E9,$F2,$F1,$E1,$F5,$E5,$F9,$FD,$ED,$92,$91,$81,$95,$85,$99,$9D
         .DB $8D,$89,$34,$24,$3C,$2C,$74,$64,$9E,$9C,$14,$1C,$04,$0C
;
;Directive table:
DIRTAB   .DB "ABGILOPQRSTUWXY"
;
;Directive jump table
DJTAB    .DW AARG
         .DW ABYTE
         .DW AGO
         .DW KEYCONV
         .DW LIST
         .DW REORIG
         .DW APRG
         .DW AQUERY
         .DW APRSTAT
         .DW ASRG
         .DW TEXT
         .DW RECODE
         .DW WORD
         .DW AXRG
         .DW AYRG
;
;END OF ASSEMBLER
;
;
;LCD interface for 2x16 character module (un-commentize the following for LCD module support)*************************
;LCDINIT  LDA  #$38
;         STA  LCDCOM
;         STA  LCDCOM
;         STA  LCDCOM
;         LDA  #$06
;         JSR  LCDWCOM
;         LDA  #$0E
;         JSR  LCDWCOM
;         LDA  #$01
;         JSR  LCDWCOM
;         LDA  #$80
;         JSR  LCDWCOM
;         RTS
;LCDWCOM  JSR  LCDBUSY
;         STA  LCDCOM
;         RTS
;LCDCOUT  JSR  LCDBUSY
;         STA  LCDDATA
;         RTS
;LCDBUSY  PHA
;LCDLOOP  LDA  LCDCOM
;         AND  #$80
;         BNE  LCDLOOP
;         PLA
;         RTS
;LCDPRMT  LDX  #$00
;LCDMORE  LDA  LCDTEXT,X
;         JSR  LCDCOUT
;         INX
;         CPX  #$38 ;TOTAL LCD DATA BUFFER
;         BNE  LCDMORE
;         RTS
;         JSR  LCDINIT
;         JSR  LCDPRMT
;         NOP  ;REPLACE THESE NOPS WITH
;         NOP  ;JMP $400 FOR "USER" VERSION
;         NOP  ;OF SyMon III 
;         RTS
;LCDTEXT  .DB  "S/O/S SyMon III "
;         .DB  "                "
;         .DB  "        "
;         .DB  "version 06.07.07"
;         .DB  $00
;
;QUERY command:
         .ORG $FC00
;NOTE: hard-coded prompt string pointers here
QUERY    JSR  COUT
         LDA  #$0E
         STA  INDEX
         LDA  #$FC
         STA  INDEXH
         JMP  VQUERY
;QUERY string (located beginning at address $FC0E):
         .DB "$E138 CHIN    "
         .DB "$E15A COUT",$0D,$0A
         .DB "$E157 COUT2   "
         .DB "$E18B CROUT",$0D,$0A
         .DB "$E188 CR2     "
         .DB "$E5A0 SPC2",$0D,$0A
         .DB "$E59D SPC4    "
         .DB "$E3F0 PRASC",$0D,$0A
         .DB "$E183 DOLLAR  "
         .DB "$E3FD PRBYTE",$0D,$0A
         .DB "$E40E PRINDX  "
         .DB "$E41B PROMPT",$0D,$0A
         .DB "$E10D BEEP    "
         .DB "$E198 DELAY1",$0D,$0A
         .DB "$E19D DELAY2  "
         .DB "$E578 SET",$0D,$0A
         .DB "$E57A TIMER   "
         .DB "$E44D RDLINE",$0D,$0A
         .DB "$E115 BN2ASC  "
         .DB "$E000 ASC2BN",$0D,$0A
         .DB "$E1B9 HEXIN   "
         .DB "$E4C6 SAVREGS",$0D,$0A
         .DB "$E4B0 RESREGS "
         .DB "$xxxx LCDINIT",$0D,$0A
         .DB "$xxxx LCDOUT  "
         .DB "$E357 INCINDEX",$0D,$0A
         .DB "$E5D5 DECIN   "
         .DB "$E97A PROMPT2",$0D,$0A
         .DB "$E1B3 HEXIN2  "
         .DB "$E1B7 HEXIN4",$0D,$0A
         .DB "$EB55 NMON    "
         .DB "$EAF4 COLDSTRT",$0D,$0A
         .DB "6551:$8000 "
         .DB "LCD:$9000 "
         .DB "8255:$A000"
         .DB $00
;
;STORLF subroutine: This is a patch that is part of the "Z" (text editor) command;
; it stores a [LINEFEED] after [RETURN] when [RETURN] is struck.
STORLF    JSR  COUT      ;Send [RETURN] to terminal 
          JSR  INCINDEX  ;Increment edit memory address pointer
          LDA  #$0A
          STA  (INDEX),Y ;Store [LINEFEED] in indexed edit memory
          JSR  COUT      ;Send [LINEFEED] to terminal
          RTS            ;Done STORLF subroutine (patch), RETURN   
;
;
;Prompt strings for [F]MFILL, [M]MOVER, [K]SRCHBYT, [L]SRCHTXT, [CNTL-W]WIPE commands and SUB-ASSEMBLER: 
         .ORG $FE00
         ;String:                   String number:
         .DB $00
         .DB $0D, $0A               ;$01
         .DB "[ESC] key exits,"
         .DB " any other to"
         .DB " proceed"
         .DB $00
         .DB $0D, $0A               ;$02
         .DB "fill start: "
         .DB $00
         .DB $0D, $0A               ;$03
         .DB "length: "
         .DB $00
         .DB $0D, $0A               ;$04
         .DB "value: "
         .DB $00
         .DB $0D, $0A               ;$05
         .DB "move source: "
         .DB $00
         .DB $0D, $0A               ;$06
         .DB "destination: "
         .DB $00
         .DB $0D, $0A               ;$07
         .DB "Find "
         .DB $00
         .DB "text: "               ;$08
         .DB $00
         .DB "bytes: "              ;$09
         .DB $00
         .DB "found "               ;$0A
         .DB $00
         .DB "not "                 ;$0B
         .DB $00
         .DB "at: $"                ;$0C
         .DB $00
         .DB $0D, $0A               ;$0D
         .DB "Searching.."
         .DB $00
         .DB $0D, $0A               ;$0E
         .DB "N=Next? "
         .DB $00
         .DB "Byte: "               ;$0F
         .DB $00
         .DB "Word: "               ;$10
         .DB $00
         .DB "Origin: "             ;$11
         .DB $00
         .DB "Assembler:"           ;$12
         .DB $00
         .DB "Recode"               ;$13
         .DB $00
         .DB "List: "               ;$14
         .DB $00
         .DB " <-Offset = low byte" ;$15
         .DB $00
;
;Monitor prompt strings:
         .ORG $FF00
         ;String:                   String number:
         .DB $00
         .DB "Version 06.07.07"     ;$01
         .DB $00
         .DB "[CNTL-A] runs "       ;$02
         .DB "assembler"
         .DB $00
         .DB "S/O/S SyMon "         ;$03
         .DB "(c)1990 B.Phelps"
         .DB $00
         .DB " "                    ;$04 (placeholder)
         .DB $00
         .DB "Areg:$"               ;$05
         .DB $00
         .DB $0D, $0A               ;$06
         .DB "HEX: $"
         .DB $00
         .DB " "                    ;$07 (placeholder)
         .DB $00
         .DB "DEC: "                ;$08
         .DB $00
         .DB $0D, $0A               ;$09
         .DB "Download:"
         .DB $00
         .DB "Xreg:$"               ;$0A
         .DB $00
         .DB "SyMon III"            ;$0B
         .DB $00
         .DB "Monitor:"             ;$0C
         .DB $00
         .DB "adrs+"                ;$0D
         .DB $00
         .DB "Yreg:$"               ;$0E
         .DB $00
         .DB $0D, $0A               ;$0F
         .DB "Upload "
         .DB $00
         .DB "Stack pointer:$"      ;$10
         .DB $00
         .DB "Address: "            ;$11
         .DB $00
         .DB "Length: "             ;$12
         .DB $00
         .DB $0D, $0A               ;$13
         .DB "10 Seconds"
         .DB $00
         .DB "Processor status:$"   ;$14
         .DB $00
         .DB "Wipe RAM?"            ;$15
         .DB $00
;
;6502 Vectors:
         .ORG $FFFA
         .DW $0300    ;NMI
         .DW COLDSTRT ;RESET
         .DW INTERUPT ;IRQ





         .END

