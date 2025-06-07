ADDI R2 R0 -5      # R2 = 0 + 5
ANDI R6 R1 3      # R6 = R2 & 3 (should be 1)
SUB R5 R3 R2      # R5 = R3 - R2 (should be 2)
BNE R1 R2 2       # if R1 != R2, skip next 2 instructions
ORI R7 R2 8       # R7 = R2 | 8 (should be 13)
SLL R8 R2 1       # R8 = R2 << 1 (should be 10)
SRL R9 R3 1       # R9 = R3 >> 1 (should be 3)

