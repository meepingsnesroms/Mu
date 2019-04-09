*-----------------------------------------------------------
* Title      :patchMemChunkNew
* Written by :meepingsnesroms
*-----------------------------------------------------------
    ORG    $1000
START:                  ; first instruction of program

validateSize:
    and.l #3, d0
    beq sizeClean
    add.l #1, d0
    bra validateSize
sizeClean:

    END    START        ; last line of source

*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
