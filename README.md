Assembler and emulator for a made-up CPU.

Hello world program which uses default emulator device that implements stdout when writing to `0xF004` and emulator halt when writing to `0xF000`.
```arm
#define EMU_HALT     0xF000
#define EMU_OUT      0xF004

section .data 0x20

msg:
    byte[] "Hello, Sailor!\n\0"

section .text 0x0
main:
    li r1, 0
beg:
    ldb r0, [r1 + msg]
    jz r0, end
    call putchar
    li r0, 1
    add r1, r1, r0
    jmp beg
end:
    stb r0, [#EMU_HALT]
    // The pseudo instruction 'hlt' can be used instead

putchar:
    stb r0, [#EMU_OUT] 
    ret
```

The following is printed when emulating the program:
```bash
[emarioo@lapis:~/dev/lwmcore]$ make && bin/lwm examples/programs/hello.s -e
Core entry: 0
Core count: 2
Core mode:  16-bit
ROM 48 bytes (0x30)
 0x0: 00 01 00 2F 
 0x4: 06 0C 19 2B 
 0x8: 00 0B 1E 0D 
 0xc: 00 00 01 0E 
 0x10: 01 01 00 1B 
 0x14: EE 36 00 00 
 0x18: F0 36 01 04 
 0x1c: F0 00 00 21 
 0x20: 48 65 6C 6C 
 0x24: 6F 2C 20 53 
 0x28: 61 69 6C 6F 
 0x2c: 72 21 0A 00 
Start emulator
Hello, Sailor!
Stop emulator
CORE[0]:
 mode: 16-bit
 pc: 0x15
 tickCounter: 124
  r0: 0 (0x0)   r1: 15 (0xf)   r2: 0 (0x0)   r3: 0 (0x0) 
  r4: 0 (0x0)   r5: 0 (0x0)   r6: 0 (0x0)   r7: 0 (0x0) 
  r8: 0 (0x0)   r9: 0 (0x0)  r10: 0 (0x0)  r11: 0 (0x0) 
 r12: 0 (0x0)   tp: 0 (0x0)   lr: 12 (0xc)   sp: 0 (0x0) 
   CRSTATUS: 0 (0x0)        CRVB: 0 (0x0)        CRPT: 0 (0x0)       CRISP: 0 (0x0) 
  CRESTATUS: 0 (0x0)       CREPC: 0 (0x0)       CRESP: 0 (0x0)     CRCAUSE: 0 (0x0) 
    CRFAULT: 0 (0x0)     CRCPUID: 0 (0x0)  CRTIMERCMP: 0 (0x0)    CRKERNEL: 0 (0x0) 
       cr12: 0 (0x0)        cr13: 0 (0x0)        cr14: 0 (0x0)        cr15: 0 (0x0) 
CORE[1] Not started
```

# Dependencies and building
|Name|Version|Description|
|-|-|-|
|GNU Make |4.4.1  |Tested with this version, older versions probably work too|
|GCC      |13.4.0 |same as above|
|Python   |3.11   |same as above. Python is only used for testing.|

```bash
# Ubuntu
sudo apt install python3 gcc make

# NixOS (shell.nix)
nix-shell
```

```bash
# Clone and build assembler and emulator (bin/lwm contains both)
git clone https://github.com/Emarioo/lwmcore
cd lwmcore
make -j
bin/lwm -v
bin/lwm -h
```

```bash
# Run a program
bin/lwm temp.s -e

# Run tests
tests/run.py
```

# Should be finished before v1.0

## Fundamentals
- [x] Assembler only handles labels for call. Do same for jump and conditional jumps.
- [x] Read tick and timer.
- [x] Exception vector and illegal operations jumping to it.
- [X] Interface for firmware and hardware devices (*HardwareDevice* MMIO, UART, IC, EMU).
- [x] Interrupts. Needs redesign to handle multiple cores.
- [x] Multiple cores.
- [x] Page tables
- [x] User mode, syscall.
- [x] Atomic instructions. (xadd, cas)
- [x] Local jumps in assembler `.loop ->  lock.loop`
- [x] Include directive in assembler.
- [x] Platform config.
- [x] Test some 32-bit and 64-bit mode.
- [x] Make a test suite for various instructions and exception, paging semantics. All of the 16/32/64-bit CPUs should be tested.
  
## Polish
- [ ] Put MMIO address definitions in a file where programs can include them so i don't have to update them everywhere.
- [ ] Operand checks in assembler. Using immediate where only register is allowed for example.
- [ ] Resolve TODOs in code.
- [ ] Revision of instruction encodings. Explore automatic encoder/decoder C code generator using schemes.
- [ ] Stabilize platform devices. Interrupt controller, platform device (multicore and IPI), UART, Emulator device.
      Apart from the implementation a document explaining the device is needed. Also tests and assembly file with macros for
      MMIO addresses and bit fields.
- [ ] Performance metrics in the emulator. Currently only shows instructions per second at the end, maybe that's all we want?

## Documentation
- [ ] Finish CPU ISA markdown.
- [ ] Finish assembler guide/spec?. preprocessor, labels, sections.
- [ ] Finish emulator guide. Arguments and platform config. How to make and emulate HardwareDevices through MMIO.
- [ ] Finish a Getting Started Guide to the ecosystem: emulator, assembler, CPU instructions, devices.

## Test cases

These tests are not thorough and do not cover edge cases. We add rudimentary tests where
many instructions and semantics are tested to any degree, bare minimum for testing.

### Basic instructions
- [x] Arithmetic
- [x] Memory addressing
- [x] Memory load/store byte, short, long, quad, sign-extension
- [x] Control flow, call, jmp, conditional jump
- [x] CPU features, just register bytesize for now
- [x] Atomic instructions, xadd, cas, includes some testing for multicore booting
- [x] Save and restore instructions

### Exceptions
- [x] Illegal instruction
- [x] Debug breakpoint
- [x] Division by zero
- [x] Misaligned Access (test various instructions including save/restore? atomics, normal load/store)
- [ ] Double fault (not super important? we can test later? how do we test it?)

### Interrupts
- [x] Timer, includes rdtick instruction
- [x] Enable/disable interrupts
- [ ] External interrupts from some device. Timer interrupt is CPU internal and doesn't count. We cannot use keyboard interrupt because it requires user.

### Paging
- [x] Page tables for 16/32/64-bit CPUs.
- [x] Enable/disable paging (we test enabling, not disabling)
- [x] Page fault on READ-only
- [x] Page fault on WRITE
- [x] Page fault on EXECUTE
- [x] Page fault on USER

### User mode
- [x] Protection fault, on all instructions that should be privileged.
- [x] Running code in user mode. Page fault with USER BIT set.
- [x] SYSCALL
- [x] VRET

# Work for the future
- [x] Display device. (uart can be used for keyboard input)
- [ ] Disk device
- [ ] Implement decoder in logic world.
- [ ] Floating point arithmetic
- [ ] Just-In-Time transpiling from LWM to native x86_64. Appreciated when emulating compute heavy programs.
- [ ] Virtualization, VMSTART

# Documents

See specifications in [docs](/docs). (all documents are work in progress)
- [ISA](/docs/isa.md) : Details about the CPU's registers, instructions, exception and paging semantics.
- [Assembler](/docs/assembler.md) : Guide for the assembler.
- [Getting Started](/docs/guide.md) : A complete guide to start using the assembler and emulator.
- [Realisation in Logic World](/docs/logic_world.md) : How the CPU is or can be realized in Logic World (may not be feasible with the new changes).
