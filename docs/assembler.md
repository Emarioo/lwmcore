Guide of the assembler for LWM.

# Command Line Usage

At the moment assembler and emulator are merged into one. To both assemble and emulate simply use `lwm program.s -e`.

The assembler only accepts one assembly file. A program that uses many must include them in the "main" assembly file.

```bash
# Assemble and run in emulator
lwm main.s -e

# Assemble and write raw binary to a file
lwm main.s -o main.bin
# Assemble and write logic world binary to a file
lwm main.s -o main.partialworld

# Run in emulator
lwm main.bin -e

# BELOW IS WORK IN PROGRESS

# Run in emulator, loads kernel.bin at 0 and main.bin at 0x1000
lwm kernel.bin --load=main.bin,0x1000

# Physical memory of kernel
lwm kernel.bin --load=main.bin,0x1000
```

# Assembly Syntax
The assembler parses a file of ASCII characters (unicode not supported) into a binary blob.


An assembly program is built upon these:
- **Section** : Describes an address range in the final executable.
- **Label** : A name for an address at the label's location.
- **Instruction** : An instruction with a name and operands.
- **Global variable** : Described with a type and an initializer such as literal strings and numbers. Commonly prefixed by a label.
- **Include directive** : Inserts text from another assembly file.
- **Macros** : Lets you name and reuse constants and common sequences of instructions.

## Comments
The assembler supports these comments:
- `// double slashing`
- `/* multi-line comment on a single line */`
- `# is this pound symbol or hashtag?`
- `; why not semi-colon too`

The `#` character is also used for directives. A directive is defined as `#DIRECTIVE` where no immediate space follows the `#`.

## Section
Assembly programs have a default section at `0x0`. This will not collide with other sections if it is left empty. A section is defined with `section .text 0x0`. Content such as instructions and global variables will be "appended" to the current section.

The assembler is also a linker. When declaring a section you must specify at which address it should be placed in the ROM.

Examples:
```arm
section .text 0x0
section data 0x200
section ".vector" 0x100
```

You can declare a section again without the address if you wish to append more content later in the assembly file.
This is useful when a program is divided into multiple files:

```arm
# kernel.s
section .text 0x0
main:
    call setup_vector
    # ...
    hlt

section .data 0x2000
// In this case kernel doesn't  have any data but must specify it
// for vector.s or other assembly "module" files that we include.

#include "vector.s"

# vector.s

// If no address is given then an existing section will be found where instructions/data are appended to it.
section .text
setup_vector:
    # ...
    ret

section .data
vector:
    long[64]

```

## Label
A label assigns a symbolic name to a memory address. Labels can be used as jump targets or variable references.

```arm
main:
    li r0, 1
    ldh r1, [check_function]
    call r1

    call check_failure
    jnz r0, .end

.end # Local label: "main.end"
    ret


check_zero:
    li r1, 0
    jz r0, .end
    li r1, 1
.end    # Local label: "check_zero.end"
    mov r0, r1
    ret

check_function:
    long check_zero # labels can be put in initializers.

```

## Instruction
An instruction consists of a mnemonic and operands. Operands may include registers, immediate values, and memory addresses.

The [ISA](isa.md) document describes the available instructions, their operands, and the available registers.

You can find code examples in [Examples](../examples) and [Tests](../tests).

```arm
main:
    li r0, 42       # Load 42 into register 0
    li r1, 8        # Load 42 into register 1
    add r0, r0, r1  # Add r1 to r0
    jmp end         # Jump to 'end'
    li r1, 4
    shl r0, r0, r1  # Shift r0 left by 4
end:
    ret
```

**TODO**: Cover memory addressing

### Pseudo instructions

```arm
hlt // stb r0, [0xF000]

mov r0, r1 // or r0, r1, r1
```


## Global variable
Global variables store data in memory. They are defined with a type and an optional value or multiple values if type is an array. Variables without values are initialized to zero. The "values" part is called initializer.

```arm
# basic types
label: byte
label: short
label: long
label: quad

# arrays
label: byte[16]
label: short[4]
label: long[8]
label: quad[4]

# initializer
label: short 23
label: byte 48
label: byte[4] "yes\0"
label: byte[0b11] 1, 0x5, -0b4
label: byte[] "A string where length is automatically determined\0"

label: byte 'A'
label: byte[2] 'A', 'B'

label: byte[1] "samae"    # ERROR: Characters does not fit
```
**TODO**: Full suppport for escaped characters in strings. Currently only \0 \n


## Include directive
The `#include` directive inserts another assembly file at the directive's location. It is as simple as a text copy and insert. Since you cannot specify more than one assembly file to the assembler (by design), you have to use include directives to compile large assembly programs split across multiple files. Circular includes is not allowed.

The path specified in the include directive may be:
- An absolute path.
- TODO: A file relative to the current file's folder (the file containing the include).
- TODO: A file located one of the include folders specified when running the assembler.

**NOTE:** Current working directory is searched at the moment but this is likely to change in the future.

There is also `#pragma once`. Works like C.

```arm
# util.asm
add:
    add ra, rb
    ret

# main.asm
include "util.asm"

main:
    li ra, 5
    li rb, 9
    call add
    ret
```


# Macros

```arm
#define PAGE_READ 0x1

#define PUTMSG(X)
    li r0, %X%
    call putstring
    li r0, '\n'
    call putchar
#endmacro

hello:
    byte[] "Hello\n\0"

#PUTMSG(hello)
```


# Repeat

```arm
vector:
    long[32] #repeat 31, "ex_handler, " ex_handler
```
