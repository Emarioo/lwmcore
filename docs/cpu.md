Specification for Logic World 16-bit multicore processor.

# Overview

# Instruction set
The table below contains all the instructions understood by the processor.

|Assembly encoding|Description|
|:-|-|
|li *reg0*, *imm8*|Loads an 8-bit immediate into register *reg0*|
|mov *reg0*, *reg1*|Moves a 16-bit value from register *reg1* to *reg1*|
|lodw *reg0*, [*reg1*]|Moves a 16-bit value from RAM at *reg1* into register *reg1*|
|lodb *reg0*, [*reg1*]|Moves a 8-bit value from RAM at *reg1* into register *reg1*|
|strw [*reg0*], *reg1*|Moves a 16-bit value from register *reg1* to RAM at *reg0*|
|strb [*reg0*], *reg1*|Moves a 8-bit value from register *reg1* to RAM at *reg0*|
|shl *reg0*, *imm4*|Shifts the bits in register *reg0* to the left by *imm4* (0-15)|
|shr *reg0*, *imm4*|Shifts the bits in register *reg0* to the right by *imm4* (0-15)|

## Binary encoding

## Remarks
- The byte versions of load clears the upper bits of the register.
- Load immediate (`li`) does not clear the upper bits since we want to allow:
```arm
li ra, 128
shl ra, 8
li ra, 55
```

# Addresses