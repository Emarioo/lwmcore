Emulator for 16-bit Multicore computer in **Logic World**.

# Usage
Minimal example you can emulate.
```bash
# main.asm
main:
    li ra, 6
    li rb, 2
    add ra, rb
    ret

# cmd (assemble and emulate)
lwm main.asm -e
```

<!-- To run on the intended target you need to buy **Logic World** on steam. -->

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