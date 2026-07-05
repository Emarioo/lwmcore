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

# Output

```
emarioo@beast:/mnt/e/dev/lwmcore$ make && lwm examples/hello.s -e
make: Nothing to be done for 'all'.
Core entry: 0
ROM 48 bytes (0x30)
 0x0: 00 01 00 2E
 0x4: 06 0C 19 2A
 0x8: 00 0B 1D 0A
 0xc: 00 00 01 0D
 0x10: 01 01 00 1A
 0x14: EE 27 35 00
 0x18: 00 20 20 00
 0x1c: 00 00 00 00
 0x20: 48 65 6C 6C
 0x24: 6F 2C 20 53
 0x28: 61 69 6C 6F
 0x2c: 72 21 0A 00

Start emulator
Hello, Sailor!
TODO implement wfi
Stop emulator
Core state:
 mode: 16-bit
 pc: 0x15
 tickCounter: 0
  r0: 0 (0x0)   r1: 15 (0xf)   r2: 0 (0x0)   r3: 0 (0x0)
  r4: 0 (0x0)   r5: 0 (0x0)   r6: 0 (0x0)   r7: 0 (0x0)
  r8: 0 (0x0)   r9: 0 (0x0)  r10: 0 (0x0)  r11: 0 (0x0)
 r12: 0 (0x0)   tp: 0 (0x0)   lr: 12 (0xc)   sp: 0 (0x0)
   CRSTATUS: 0 (0x0)        CRVB: 0 (0x0)        CRPT: 0 (0x0)       CREPC: 0 (0x0)
    CRCAUSE: 0 (0x0)     CRFAULT: 0 (0x0)     CRCPUID: 0 (0x0)  CRTIMERCMP: 0 (0x0)
        cr8: 0 (0x0)         cr9: 0 (0x0)        cr10: 0 (0x0)        cr11: 0 (0x0)
       cr12: 0 (0x0)        cr13: 0 (0x0)        cr14: 0 (0x0)        cr15: 0 (0x0)
```

# TODO

- [x] Assembler only handles labels for call. Do same for jump and conditional jumps.
- [ ] Operand checks in assembler. Using immediate where only register is allowed for example.
- [ ] Make a test suite with some basic programs. Prime numbers, bubble sort, extensive instruction testing. Testing should be done for 16/32/64 don't forget that!
- [ ] Read tick and timer.
- [ ] Exception vector and illegal operations jumping to it.
- [ ] Page tables.
- [ ] Interrupts.
- [ ] Interface for firmware and hardware devices. Disk device?
- [ ] User mode, syscall.
- [ ] Multiple cores.
- [ ] Implement decoder in logic world.

# Documents
See specifications in [docs](/docs). (all documents are work in progress)
- [CPU](/docs/cpu.md) : Thorough details of the CPU's registers, special addresses, clock speed.
- [Assembly](/docs/assembly.md) : Thorough guide of the assembly syntax.
- [Guide](/docs/guide.md) : A complete guide to using the assembler, emulator, and realising the system in **Logic World**.