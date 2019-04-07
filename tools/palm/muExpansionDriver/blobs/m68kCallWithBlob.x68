*-----------------------------------------------------------
* Title      :m68kCallWithBlob
* Written by :meepingsnesroms
*-----------------------------------------------------------
    ORG    $1000
START:                  ; first instruction of program

    ;setup fake OS enviroment
    lea stack, sp
    move.l #$12345678, a0
    move.l #$87654321, d0

    ;setup stack
    sub.l #2, sp
    move.w #1, (sp)
    pea (6)
    pea stackBlob
    pea functionTest
    jsr m68kCallWithBlobFunc
    SIMHALT
   
;uint32_t m68kCallWithBlobFunc(uint32_t functionAddress, uint32_t stackBlob, uint32_t stackBlobSize, uint16_t returnA0);
;Extract:vvv
m68kCallWithBlobFunc:
    link a6,#0
    movem.l d1-d2/a1-a3,-(sp)
    move.l 8(a6), a3 ;function
    move.l 12(a6), a1 ;stackBlob
    move.l 16(a6), d1 ;stackBlobSize
    move.w 20(a6), d2 ;returnA0
    move.l sp, a2
    sub.l d1, a2
copy:
    cmp.l a2, sp
    beq done
    move.w (a1), (a2)
    add.l #2, a1
    add.l #2, a2
    bra copy
done:
    ;update SP with new stack
    sub.l d1, sp
    jsr (a3)
    add.l d1, sp
    ;new stack has been used and removed, SP is safe again
    tst.w d2
    beq useD0
    move.l a0, d0
useD0:
    movem.l (sp)+,d1-d2/a1-a3
    unlk a6
    rts
;Extract:^^^

functionTest:
    ;test that calling works and the return value respects returnA0
    move.l #$10101010, a0
    rts

;Stack
    ORG    $2000
stack:
    dc.w $0,  $0,  $0
    
;Variables
    ORG    $3000
stackBlob:
    dc.w $1,  $2,  $3

    END    START        ; last line of source

*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
