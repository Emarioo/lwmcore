Assembler, emulator, and ROM writer for 16-bit Multicore computer in **Logic World**.

To run in Logic World you need at least version `0.92` (currently preview beta version) where subassemblies where added.

# Usage
This project has two uses.
1. Assembling and emulating a program based on this architecture [CPU](/docs/cpu.md).
2. Reading a Logic World subassembly file and setting the state of the switches based on a binary file.

The binary data can be provided through arguments `lwm mydata.bin --rom myrom.partialworld` or through the assembled program.

Minimal example you can assemble, emulate, and run in Logic World.
```bash
# main.asm
main:
    li ra, 6
    li rb, 2
    add ra, rb
    ret
```
These are some common commands.
```bash
# assemble and emulate
lwm main.asm --emulate

# assemble to binary
lwm main.asm -o main.bin

# assemble to logicworld subassembly
lwm main.asm --rom test
```

The subassembly will automatically be placed in `C:/Program Files (x86)/Steam/steamapps/common/Logic World/subassemblies`. Prefix path with `./` or an absolute path to place subassembly in a different location than the default.

You can use `lwm main.asm --rom main.partialworld` if you just want the file without the metadata. This file will not be placed in the `Logic World` subassemblies folder (it will be placed in the current working directory).

# Building/Installing
- Install GCC
- Clone repo
- Compile with `build.py` (should compile to `./lwm`)
- Run with `lwm`

You may want to set environment variables to the folder with `lwm.exe`.

# Documents
See specifications in [docs](/docs). (all documents are work in progress)
- [CPU](/docs/cpu.md) : Thorough details of the CPU's registers, special addresses, clock speed.
- [Assembly](/docs/assembly.md) : Thorough guide of the assembly syntax.
- [Guide](/docs/guide.md) : A complete guide to using the assembler, emulator, and realising the system in **Logic World**.