Specification of the assembly for Logic World 16-bit multicore processor.

# Overview

## Usage
Use `lwm <in_path> -o <out_path>` to assemble file `in_path` and generate a binary at `out_path`.

The file extension of `in_path` and `out_path` should be `.asm` and `.bin` respectively.

## Input and output
The assembler parses a file of ASCII characters (unicode not supported) into a binary blob.

# Syntax

An assembly program is built upon these:
- **Block** : Describes an address range in the final executable. Marked with `ORG`.
- **Label** : A name for an address at the label's location.
- **Instruction** : An instruction with a name and operands.
- **Global variable** : Described with a type and an initializer such as a literal strings and numbers. Commonly prefixed by a label.
- **Include directive** : Inserts text from another assembly file.

## Comments
The assembler supports these comments:
- `// double slashing`
- `/* multi-line comment on a single line */`
- `# is this pound symbol or hashtag?`
- `; why not semi-colon too`

## Block
All assembly programs have a default block starting at address `0x0000`. A new block at a specific address can be created with `ORG 0x8000`. Content such as instructions and global variables will be "appended" to the previous block.

### Example:
```arm
ORG 0x8000
main:
    li ra, 5    # located at 0x8000
    add ra, ra  # located at 0x8002

var: int16[4]   # located at 0x8004
```

## Label
A label assigns a symbolic name to a memory address. Labels can be used as jump targets or variable references.

### Example:
```arm
start:
    jmp start  # infinite loop
```

## Instruction
An instruction consists of a mnemonic and operands. Operands may include registers, immediate values, and memory addresses.

The [CPU](/docs/cpu.md#instruction-set) document describes the available instructions, their operands, and the available registers.

### Example:
```arm
main:
    mov ra, 42   # Load 42 into register $ra
    add ra, rb   # Add $rb to $ra
    jmp end      # Jump to 'end'
    shl ra, 4
end:
    ret
```

**TODO**: Cover special memory arithmetic syntax, if we have it?

**TODO**: Show how to reference a variable.

## Global variable
Global variables store data in memory. They are defined with a type and an optional value or multiple values if type is an array. Variables without values are initialized to zero. The "values" part is called initializer.

### Examples:
```arm
# basic types
label: char
label: int8
label: int16

# arrays
label: char
label: char[16]
label: int8[4]
label: int16[8]

# initializer
label: int16 23
label: char 48
label: char[6] "yes"        # chars are zeroed at the end
label: int8[0b11] { 1, 0x5, -0b4 }
label: char[] "A string where length is automatically determined"

label: char 'A'             # TODO: NOT IMPLEMENTED YET!
label: char[6] { 'A', 'B' } # TODO: NOT IMPLEMENTED YET!
# TODO: Support escaped characters in strings.

# TODO: edge cases and errors
label: char[4] "same"     # ERROR: Null terminated character does not fit
label: char[1] "samae"    # ERROR: Characters does not fit
```

**NOTE:** Undefined behaviour if you specify an array with a size of more than 10000 (100000 or more might be fine but we haven't verified it)

## Include directive
The `#include` directive inserts another assembly file at the directive's location. It is as simple as a text copy and insert. Since you cannot specify more than one assembly file to the assembler (by design), you have to use include directives to compile large assembly programs split across multiple files.

The path specified in the include directive may be:
- An absolute path.
- TODO: A file relative to the current file's folder (the file containing the include).
- TODO: A file located one of the include folders specified when running the assembler.

**NOTE:** Current working directory is searched at the moment but this is likely to change in the future.

### Example:
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

**NOTE:** We don't allow multiple assembly files to the assembler program because one initial assembly file keeps your project better organized. Specify 5-10 assembly files on the command line is annoying. Also no need for makefiles.

# Future features
- C like macros (at least without arguments)
