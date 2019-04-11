*-----------------------------------------------------------
* Title      :patchPrvPtrResize
* Written by :meepingsnesroms
*-----------------------------------------------------------
    ORG    $1000
START:                  ; first instruction of program

    ; Patch to address 0x2123C of palmos41-en-m515.rom
    move.l $C(a6), d0
    and.l #$FFFFFFFC, d0
    cmp.l $C(a6), d0
    beq done
    addq.l #4, d0
done:
    addq.l #8, d0
    addq.w #8, sp

    END    START        ; last line of source



*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
