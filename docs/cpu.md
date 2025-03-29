Specification for Logic World 16-bit multicore processor.

# Overview
Clock speed
Memory bus?
IO controller?
RAM access? (stack)
Flash access? (binary blob)

Little endian

# Instruction set
The table below contains all the instructions understood by the processor.

|Assembly encoding|Description|
|:-|-|
|li *reg0*, *imm8*|Load an 8-bit immediate into register *reg0*|
|mov *reg0*, *reg1*|Move a 16-bit value from register *reg1* to *reg0*|
|shl *reg0*, *imm4*|Shift the bits in register *reg0* to the left by *imm4* (0-15)|
|shr *reg0*, *imm4*|Shift the bits in register *reg0* to the right by *imm4* (0-15)|
|lodw *reg0*, [*reg1*]|Load a 16-bit value from RAM at *reg1* into register *reg0*|
|lodb *reg0*, [*reg1*]|Load a 8-bit value from RAM at *reg1* into register *reg0*|
|strw [*reg0*], *reg1*|Store a 16-bit value from register *reg1* to RAM at *reg0*|
|strb [*reg0*], *reg1*|Store a 8-bit value from register *reg1* to RAM at *reg0*|
|lodw *reg0*, [sp + offset]|Load a 16-bit value from RAM at *[sp + offset]* into register *reg0*|
|lodb *reg0*, [sp + offset]|Load a 8-bit value from RAM at *[sp + offset]* into register *reg0*|
|strw [sp + offset], *reg0*|Store a 16-bit value from register *reg0* to RAM at *[sp + offset]*|
|strb [sp + offset], *reg0*|Store a 8-bit value from register *reg0* to RAM at *[sp + offset]*|


add sub and or xor cmp
jmp jeq jne jlt jgt
call ret

**NOTE:** Shift operations are logical shifts. Arithmetic shifts (sign-extend) are not available.

## Registers
|Assembly encoding|Description|
|:-|-|
|ra|General register 0|
|rb|General register 1|
|rc|General register 3|
|rd|General register 4|
|re|General register 5|
|rf|General register 6|
|rg|General register 7|
|rh|General register 8|
|-|Reserved 9-13|
|sp|Stack pointer (14)|
|pc|Program counter (15)|

## Binary encoding
Each instruction is 2 bytes that stores the opcode and operands. These are the forms.

**Basic**
- `[ OPCODE 4 | REG 4 | IMMEDIATE 8 ]` - li, jeq, jne, jlt, jgt
- `[ OPCODE 4 | IMMEDIATE 12 ]` - jmp, call

**Extended**
- `[ OPCODE 8 | REG 4 | REG 4 ]` - lodw, lodb, strw, strb, add, sub, and, or, xor, cmp
- `[ OPCODE 8 | REG 4 | IMMEDIATE 4 ]` - shl, shr
- `[ OPCODE 8 ]` - ret

**Special**
- `[ OPCODE 5 | REG 3 | OFFSET 8 ]` - lodw, lodb, strw, strb (with [sp + offset])

The instruction's opcode is stored in the upper bits of the 16-bit instruction.
The encoding is constructed like this in C on a little endian machine `u16 word = (opcode << 8)reg0<<4) | reg1`.

We do this because the instruction opcode values increments by one for the next instructions. If we used the lower bits to differentiate between Special and Extended opcodes then instructions would increment by 2 or 4.

**Encoding for opcodes:**

|Instruction|Basic opcode (4b)|
|-|-|
|li      |0b0000 (0)|
|jeq     |0b0001 (1)|
|jne     |0b0010 (2)|
|jlt     |0b0011 (3)|
|jgt     |0b0100 (4)|
|jmp     |0b0101 (5)|
|call    |0b0110 (6)|
|Reserved|0b0111 (7)|

|Instruction|Special opcode (5b)|
|-|-|
|lodw     |0b10000 (16)|
|lodb     |0b10001 (17)|
|strw     |0b10010 (18)|
|strb     |0b10011 (19)|
|Reserved |0b10100 - 0b11111 (20-31)|

|Instruction|Extended opcode (8b)|
|-|-|
|lodw     |0b1100_0000 (192)|
|lodb     |0b1100_0001 (193)|
|strw     |0b1100_0010 (194)|
|strb     |0b1100_0011 (195)|
|add      |0b1100_0100 (196)|
|sub      |0b1100_0101 (197)|
|and      |0b1100_0110 (198)|
|or       |0b1100_0111 (199)|
|xor      |0b1100_1000 (200)|
|cmp      |0b1100_1001 (201)|
|ret      |0b1100_1010 (202)|
|shl      |0b1100_1011 (203)|
|shr      |0b1100_1100 (204)|
|Reserved |0b1100_1101 - 0b1111_1111 (205-255)|

**NOTE:** The first bits of Special and Extended opcode are 1 to differentiate the different forms and length of opcode.

**Encoding for registers:**
|Register|Encoding (4b)|
|-|-|
|ra      |0b0000 (0)|
|rb      |0b0001 (1)|
|rc      |0b0010 (2)|
|rd      |0b0011 (3)|
|re      |0b0100 (4)|
|rf      |0b0101 (5)|
|rg      |0b0110 (6)|
|rh      |0b0111 (7)|
|Reserved|0b1000 - 0b1101 (8 - 13)|
|sp      |0b1110 (14)|
|pc      |0b1111 (15)|

**Encoding for load/store type:**
|Instruction|Encoding (2b)|
|-|-|
|lodw    |0b00 (0)|
|lodb    |0b01 (1)|
|strw    |0b10 (2)|
|strb    |0b11 (3)|

## Remarks
- The byte versions of load clears the upper bits of the register.
- Load immediate (`li`) does not clear the upper bits since we want to allow:
```arm
li ra, 128
shl ra, 8
li ra, 55
```

# Stack
No push and pop instructions. There is still a **stack pointer** that grows downwards.

Your program should set up the **stack pointer** to address 0xFF or wherever.
**Program counter** starts at 0x0 but you can set this up differently if you need to.


# Hardware registers
Write to specific memory addresses to interract with I/O and display.
Read to certain address to get button/keyboard input.

# Addresses
The CPU can access memory from [0 - 65565].