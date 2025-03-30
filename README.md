Assembler and emulator for 16-bit Multicore computer in **Logic World**.

To run in Logic World you need at least version `0.92` (currently preview beta version) where subassemblies where added.

# Usage
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

The subassembly will automatically be placed in `C:/Program Files (x86)/Steam/steamapps/common/Logic World/subassemblies`. You can use `--logic-world-path` flag to choose a different location.

Prefix path with `./` or an absolute path to place subassembly in a different location than the default.

You can use `lwm --rom main.asm -o main.partialworld` if you just want the file without the metadata.

**TODO:** Create and upload Logic World 16-bit multicore processor.

# Building
- Install GCC
- `./build.py`
- `./lwm.exe`

# Documents
See specifications in [docs](/docs). (all documents are work in progress)
- [CPU](/docs/cpu.md) : Thorough details of the CPU's registers, special addresses, clock speed.
- [Assembly](/docs/assembly.md) : Thorough guide of the assembly syntax.
- [Guide](/docs/guide.md) : A complete guide to using the assembler, emulator, and realising the system in **Logic World**.