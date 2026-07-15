
Instruction Set Architecture for LWM (Logic World Machine)

# Introduction to CPU Family and features

The ISA is designed as a family of processors:

|Family|General Registers|Address Space|
|-|-|-|
|16-bit|16|22-bit (4 MB)|
|32-bit|32|32-bit (4 GB)|
|64-bit|32|48-bit (256 TB)|

The architecture supports:

- Exceptions and interrupts
- Paging
- User space
- Multicore and atomics

Additional extensions:

- (TBD) Floating point arithmetic
- (TBD) Vector extensions (SIMD)

Other details:
- Little endian
- Variable-length instructions


All addresses in control registers are physical, not virtual.
Assembly examples assumes identity mapped pages or disabled paging.

# Memory and paging

A page is 4096 bytes (12 bits).

Each page entry has an upper part and a lower part. The upper part refers to a physical address aligned by 4 KiB. The lower part (12 bits) is used for flags (present,user,write,execute,caching). The size of the upper part is based on the bit-width of the CPU.

@TODO When switching processes and flushing TLB we want process identifiers attached to each entry
in the TLB. If current process identifiers differs from what's cached in TLB then that page is flushed.
Current process identifier could be a control register or in the low bits of the page table control register.



## 16-bit

- 22-bit address space, 4 MiB.
- One-level page table with 1024 page entries.
- Each entry is 32 bits where lower 22 bits are used, rest is for padding.

## 32-bit

- 32-bit address space, 4 GiB.
- Two-level page tables each with 1024 page entries.
- Each entry is 32 bits. The indexing is as follows 10-10-12 (adds up to 32)

## 64-bit

- 48-bit address space, 256 TiB.
- Four-level page tables each with 512 page entries.
- Each entry is 64 bits. The indexing is as follows 9-9-9-9-12 (adds up to 48)

## Enabling paging

All pages and page tables must be 4 KiB aligned. Lower bits are ignored.
The following assembly enables paging.

```arm
# Set root page table
lea r1, [rootPageTable] # this should be physical address of rootPageTable
mtcr CRPT, r1

# Enable paging
mfcr r1, CRSTATUS
or r1, 0x400        # @TODO The bit may have changed
mtcr CRSTATUS, r1
```

@TODO Show how to initialize page tables and flags


# Exceptions and predefined interrupts

Exceptions are generated when the CPU encounters a state that must be handled by the kernel.
The CPU will look in a vector table specified by the control register CRVB. The vector table has 64 entries where each entry is 32 bits on 16/32-bit CPU and 64 bits on 64-bit CPU. Each entry refers to a physical address the CPU will jump to when an exception occurs.

The handler itself can acquire information about the exception through these control registers:

This table is outdated 

| Register | Description |
|----------|---------|
| CREPC    | Exception return address |
| CRCAUSE  | Exception/interrupt cause |
| CRFAULT  | Page faulting address |
| CRCPUID  | Current CPU ID |

## Vectors

|Number|Name|Description|
|-|-|-|
|0|Reserved||
|1|Illegal Instruction|Unknown instruction, invalid opcode, invalid operands, addressing modes|
|2|Protection Fault||
|3|Page Fault|An address was accessed that was not mapped or accessed in a way which flags did not permit (exampple: write but page was read-only or user accessed kernel page). Paging must be enabled.|
|4|Double Fault|An exception occured while handling another exception.|
|5|Misaligned Access|Load or store instruction where address is not aligned by the width of the instruction. Cannot occur for byte loads or stores.|
|6|Syscall|Occurs when the 'syscall' instruction is executed.|
|7|Division by Zero||
|8|Debug Breakpoint|Occurs when the 'dbg' instruction is executed.|
|9|Timer interrupt|Triggered when CRTIMERCMP becomes equal to tick counter|
|10|Bus Error|Transaction for physical address could not be completed. Or similar bus device errors.|
|11|Fatal Machine Error|A serious error happened in the machine and kernel should panic.|
|12-31|Reserved||
|32-63|External interrupts||

### Illegal Instruction

Bytes at the program counter cannot be decoded into a valid instruction.

- `CREPC` will hold the program counter at which the illegal instruction begins.
- `CRESTATUS` describes details around who caused the syscall (user/supervisor).

### Protection Fault

User tried to execute a privileged instruction. Or wrong bits are set in control registers when switching to user.
Or you set the user bit in CRSTATUS directly which is not allowed. Must be done indirectly with `CRESTATUS` and `vret`.

- `CREPC` holds the program counter to the instruction that caused the fault.


### Page Fault

Only occurs when the paging bit in CRSTATUS is set.

The fault is caused by a disallowed access to a virtual address.

- `CREPC` will hold the program counter where the instruction causing the page fault begins.
- `CRFAULT` contains the virtual address that was denied.
- `CRCAUSE` describes the reason for the fault.

|CRCAUSE bit|Reason|Description|
|-|-|-|
|0|Not present|Address is not mapped in the page table.|
|1|Denied user|The CPU is in User mode and the address has the User bit cleared requiring Supervisor mode to access it.|
|2|Denied write|A write to the address was denied because write bit in page entry is cleared.|
|3|Denied execute|An instruction decode on the address was denied because execute bit in page entry is cleared.|

### Double Fault

**WIP**

Occurs when a fault is triggered inside the exception handler. CPU considers execution inside a handler from the moment the CPU
jumps into the handler to when the `vret` instruction is executed.

What if you enable interrupts inside exception handler and get one? Are we still inside exception handler? If not then if you vret
do we jump back to exception handler? Is that allowed? Can `dint` not disable interrupts if we are in exception handler until you do `vret`?

Do we consider ourselves inside an exception handler on interrupts. We should not trigger a double fault so probably not?

### Misaligned Access

An instruction was executed that requires aligned access to memory. Cannot occur for instructions that access a byte.
Will happen for instruction that write/read 32-bit integers at a time for example.

- `CREPC` holds the program counter of the instruction that caused the fault.
- `CRFAULT` contains the misaligned address.
- `CRCAUSE` describes details around who caused the syscall (user/supervisor).

|CRCAUSE bit|Reason|Description|
|-|-|-|
|0|Reserved||
|1|User|Set according to the user bit in CRSTATUS.|


### Syscall

Is triggered when the syscall instruction is executed. The jump occurs once the syscall instruction completes.

- `CREPC` will hold the program counter to the next instruction that shall be executed.
- `CRCAUSE` describes details around who caused the syscall (user/supervisor).

|CRCAUSE bit|Reason|Description|
|-|-|-|
|0|Reserved||
|1|User|Set according to the user bit in CRSTATUS.|


### Division by Zero

Is triggered when `udiv`, `sdiv`, `umod`, or `smod` divides a value by zero.

- `CREPC` holds the program counter of the instruction that caused the fault.
- `CRCAUSE` describes details around who caused the fault (user/supervisor).

|CRCAUSE bit|Reason|Description|
|-|-|-|
|0|Reserved||
|1|User|Set according to the user bit in CRSTATUS.|

### Breakpoint

Is triggered when `dbg` is executed.

- `CREPC` holds the program counter of the `dbg` instruction that caused the fault.
- `CRCAUSE` describes details around who caused the fault (user/supervisor).

|CRCAUSE bit|Reason|Description|
|-|-|-|
|0|Reserved||
|1|User|Set according to the user bit in CRSTATUS.|

### Timer Interrupt

Is triggered when CRTIMERCMP is equal to tick counter. The jump to the timer interrupt vector does not occur until
the current instruction has been completed.

- `CREPC` will hold the program counter to the next instruction that shall be executed.
- `CRCAUSE` describes details around who caused the fault (user/supervisor).

|CRCAUSE bit|Reason|Description|
|-|-|-|
|0|Reserved||
|1|User|Set according to the user bit in CRSTATUS.|

### Bus Error

Is triggered when a transaction on the bus for physical address could not be completed.

- `CREPC` holds the program counter to instruction that caused the fault, or rather during which instruction the fault occured.

### Fatal Machine Error

The machine encountered an unrecoverable error. You should log useful information before computer crashes.

- `CREPC` holds the program counter to instruction that caused the fault, or rather during which instruction the fault occured.

## Setting up exceptions

Setup exceptions by defining a vector table with 64 32-bit entries.
Then load it in vector table control register.
Then add addresses to handlers in the vector table.


```arm
vectorTable:
    .fill 64, 4, 0   # count=64 entrySize=4 value=0

setupExceptions:
    # Load vector table into control register
    lea r1, [vectorTable] 
    mtcr CRVT, r1

    # Add handler for illegal instruction
    lea r1, [handler_illegalInstruction]
    stl [vectorTable + 1 * 4], r1

    ret

handler_illegalInstruction:

    # @TODO Show how to read information about exception through
    #   control registers. Example: cause, pc, fault address

    vret

```


# Interrupts

Interrupts use the same vector table as exceptions.
The entries 32-63 are reserved for external interrupts from hardware.

To enable interrupts on vector 32 that number should be given to the hardware device through MMIO. The exact way depends on the hardware.

@TODO Where does interrupt routing, priority, masking come into play? Interrupt controller. Firmware thing?

@TODO Inter-process interrupts?

## Enabling interrupts

First ensure a vector table is loaded. Already done if you enabled exceptions.
Then place a handler in the vector table at the extern interrupt number (32 for example).

```arm

// void setInterruptHandler(int vectorIndex, void* handler)
setInterruptHandler:
    stl [vectorTable + r1 * 4], r2
    ret

handler_keyboard:
    ...
    ret

setupKeyboard:
    push lr

    li r1, 32  // vector number

    stl [MMIO_KEYBOARD_INTERRUPT_NR], r1

    lea r2, [handler_keyboard]
    call setInterruptHandler

    pop lr
    ret

```

# User and supervisor mode

The CPU starts in supervisor mode and can switch to user mode by setting control registers and executing `vret`.
Execution in user mode is restricted to normal instructions and only pages marked user can be accessed. Executing privileged instructions such as `vret` and `mfcr` will cause a **Protection Fault**.

Switching to user mode with paging or interrupts disabled will cause **Protection Fault**. Otherwise user mode can never be interrupted or can access all physical memory.

Use `save` and `restore` instructions to save general purpose registers. Useful when context switching.


# Tick Counter and Timer

The tick counter is a 64-bit, monotonic, fixed frequency counter. It cannot be modified during execution. Only a computer reboot will reset it.
The tick unit does not represent a cycle.

The timer is a compare operation that continously compares a target value to the tick counter which triggers an interrupt when reached.
If you know the frequency of the tick counter then you can set a target such that after 10ms and interrupt is triggered.

The tick counter is read in one atomic instruction which is important for 16-bit and 32-bit CPUs which require two or more registers. There are three operand encodings for each bit width.

```arm
rdtick r1
# r1 = bits 63:0

rdtick r1, r2
# r1 = bits 31:0
# r2 = bits 63:32

rdtick r1, r2, r3, r4
# r1 = bits 15:0
# r2 = bits 31:16
# r3 = bits 47:32
# r4 = bits 63:48
```

The timer's compare value is set through a control register.

```arm
#define TICKS_PER_MS 1000000 // should be measured or retrieved from firmware or cpufeat?

rdtick r0
add r0, 10 * TICKS_PER_MS
mtcr CRTIMERCMP, r0
```


# Booting and Multicore

The platform specifies where the first core starts executing. For example: 0x0 or 0x1000.

The first core to start may not be index 0. This is important in case the first core or all but one core are defective in which case the CPU is still functional.

When booted paging and interrupts are disabled. Supervisor mode is active.

All registers except CRSTATUS, CRTIMERCMP, and CRCPUID are in an undefined state.


## Core index
```arm
mfcr r1, CRCPUID
```

## Booting other cores
Platform specifies through MMIO how many cores exist and how
to start them. Below is an example.
 
```arm
lea r1, [core_entry]
stw [PLATFORM_CORE_ENTRY],   r1
stw [PLATFORM_CORE_INDEX],   1 // We assume there is a second core and it's not the current core
stw [PLATFORM_CORE_COMMAND], 7 // boot core command
```

# Atomic Instructions

Atomic
- Add, sub
- Compare and swap

Memory barriers and fences

# CPU Features

The `cpufeat r0, r1` instruction is used to query the CPU for information. It is the best way to figure out the byte size of registers and whether floating point instructions are supported. The second operand specifies the **Feature ID** and the first operand is the result, set based on the feature ID see table below.

Feature IDs not supported or known by the CPU returns 0. The same applies for bitfields, they are cleared.

|Feature ID|Name|Description|
|-|-|-|
|0|CPUFEAT_REG_BYTES|Result is the byte size of a register. 2/4/8 for 16/32/64-bit CPU respectively.|
|1|CPUFEAT_FEATURES0|Bitfield of non-standard features. Reserved for future use.|
|2|CPUFEAT_FEATURES1|Reserved for future use.|

## CPUFEAT_FEATURES0
The table below describes the bits returned by the feature id.

|Bit|Name|Description|
|-|-|-|
|0|CPUFEAT_FEATURES0_FLOAT|Floating point support exists. This is not implemented at the moment.|

## Example

```arm
#include "tests/cpufeat-defs.s"

// Register's byte size
li r1, #CPUFEAT_REG_BYTES
cpufeat r0, r1
// r0 = 2, 4, 8 depending on CPU architecture.

// Check floating point support
li r1, #CPUFEAT_FEATURES0
cpufeat r0, r1
li r1, #CPUFEAT_FEATURES0_FLOAT
and r0, r0, r1
```

# Save and restore

When transitioning between kernel and user processes it is efficient to use dedicated save/restore instructions.
They will move registers to and from memory.

The save/restore has a mask that describes which extension registers to save if any. The full mask for all registers can be retrieved with `cpufeat`.


@TODO Do we have some alignment requirement for the memory address? 8-byte alignment or page alignment? 8-byte alignment makes sense.
   For SIMD instructions if we ever have 512-bit float registers then 64-byte alignment would make sense. With 32 gpr, 32 cr, 32 vector registers it would take up 2560 bytes. So it can fit within a page with room for more registers in the future.

```arm
struct Context {
    # Core general registers
    r0-r31
    cr0-cr31
    
    # Extension registers
    cr32-cr63
    v0-v31
}

# Saves registers to memory
save r0, 0x0

# Restore registers from memory
restore r0, 0x0

# The immediate is a mask for which extension registers to save if any.
```

# Virtualization (WIP)

How to specify multiple virtual cores. Multiple Guests on the same Host.

```arm


startVirtualCPU:
    push lr

    # Clear all registers, vCPU will start with these values
    mov r15, 0
    mov r14, 0
    mtcr CRCAUSE, r15
    # ...

    # Load page table that maps Guest Physical to Host Physical
    lea r0, [vmPageTable]
    mtcr CRVMPT, r0
    # Load initial program pointer
    lea r0, [0x1000]
    mtcr CRVMPC, r0

    vmstart
loop:
    # Resumes here on VM exits

    # handle trap/interrupt MMIO or whatever else.

    vmresume
    jmp loop

    pop lr
    ret

```

# Firmware

Firmware should provide these things?

## Timers

The CPU has an internal timer but firmware can provide more.

The programmable timer is platform hardware (MMIO).
The timer uses compare registers.

Kernel programs: `compare = current_tick + delta`

Hardware:
```
if (tick >= compare)
    raise timer interrupt
```

Each CPU may have its own compare register while sharing the same global tick counter.

## Real-Time Clock

Separate from the tick counter.

- Current date/time.
- Wall clock.
- Not used for scheduling.

Implemented as MMIO.

# Registers

Note that 16-bit CPU only has 16 registers.

|General Register|Description|
|-|-|
|r0 - r3|Arguments and return values|
|r4 - r7|Arguments|
|r8 - r12|Non-volatile|
|r13 - r15|Reserved (alias for sp,lr,tp)|
|r16 - r31|Non-volatile|
|sp|Stack pointer|
|lr|Link register|
|tp|Thread pointer (reserved for thread local storage)|

The program counter is an internal register which cannot be accessed.
You can do the following:
```arm
// Jump to next instruction where we set r0 = lr = pc
    call end
end:
    or r0, r0, lr
// Or this.
    lea r0, [end]
```


The control registers are modified by the kernel. It tells the CPU where to jump on interrupts and exceptions. How the page table looks like, whether
CPU runs in user mode or paging/interrupts are enabled.

|Control Register|Description|
|-|-|
| CRSTATUS | CPU status flags (supervisor/user, paging enabled, interrupt enabled) |
| CRVB | Vector base address |
| CRPT | Root page table |
| CREPC | Exception return address |
| CRCAUSE | Exception/interrupt cause |
| CRFAULT | Page faulting address |
| CRCPUID | Current CPU ID |
| CRTIMERCMP | Timer Compare Target |
| CRKERNEL | An unused register the kernel can use for whatever it wants. Useful because a user program cannot touch it. |


# Instructions

The encoding is described in a different document.

|opcode|reg0|reg1|reg2|
|-|-|-|-|
|0:7|8:10|11:15|16:20|

add, sub


|opcode|reg0|reg1|
|-|-|-|-|
|0:7|8:10|11:15|16:20|

**Three-operand encoding**
- `[ OPCODE 8 | REG 5 | REG 5 | REG 5 ]` - add, sub, mul, div, mod, and, or, xor, shl, shr

**Two-operand encoding**
- `[ OPCODE 6 | REG 5 | REG 5 ]` - not, mfcr, mtcr, cpufeat

**One-operand encoding**
- `[ OPCODE 3 | REG 5 ]` - call reg, jmp reg

**8/16/32/64-bit immediate encoding**
- `[ OPCODE 3 | REG 5 | IMM 8/16/32/64 ]` - li

**One-operand encoding**
- `[ OPCODE n | REG 5 ]` - push, pop

**pc-8/16/32 relative**
- `[ OPCODE 8 | IMM 8/16/32 ]` -  call, jmp

**One-operand pc-8/16/32 relative**
- `[ OPCODE 8 | REG 5 | IMM 8/16/32 ]` -  jnz, jz

**Two-operand pc-8/16/32 relative**
- `[ OPCODE 14 | REG 5 | REG 5 | IMM 8/16/32 ]` -  jeq, jne, jlt, jle, jgt, jge, jb, jbe, ja, jae

**No operand encoding**
- `[ OPCODE 8 ]` - ret, dbg, slow, wfi, eint, dint, vret, syscall

**tlbflush encoding**
- `[ OPCODE n | REG 5 ]` - tlbflush

**rdtick encoding**
- `[ OPCODE n | REG 5 ]` - rdtick
- `[ OPCODE n | REG 5 | REG 5 ]` - rdtick
- `[ OPCODE n | REG 5 | REG 5 | REG 5 | REG 5 ]` - rdtick

**Addressing encoding**
- `[ OPCODE n | REG 5 | ]` - lea

```arm
lea r0, [r1]
lea r0, [r1 + 0x48]
lea r0, [pc + 0x48]
lea r0, [0xC04000]
lea r0, [r1 + 16*r2 + 0x2]
lea r0, [r1 + 1*r2 + 0]
lea r0, [r1 + r2]
lea r0, [4*r1]
lea r0, [4*r1 + 0]
lea r0, [pc + 16*42]

// The general form where all parts are optional
lea r0, [r1 + scale*index + disp]
lea r0, [pc + scale*index + disp]
```

lea r0, [absolute]
[ opcode 8 | reg 5 | disp16/32 ]

lea r0, [r1]
[ opcode 8 | reg 5 | reg 5 ]

lea r0, [r1 + disp]
[ opcode 8 | reg 5 | reg 5 | imm8/16/32 ]

lea r0, [pc + disp]
[ opcode 8 | reg 5 | imm8/16/32 ]

lea r0, [r1 + r2*scale + disp]
[ opcode 8 | reg 5 | reg 5 | scale 2 | imm8/16/32 ]

[ opcode 8 | reg 5 | flags 8 | disp16/32 ]


[ opcode 8 | reg 5 | flags 8 | reg 5 | reg 5 | disp8/16/32 ]


## Basic




|Instruction format|
|-|
|add r0, r1, r2|

- Load immediate
- Load/store byte, half, long, quad
- Load effective
- Jump
- Jump conditional
- Call
- Return
- Exception return
- Add, sub, mul, div, mod
- And, or, xor, not
- Shift left/right

## Special

|Instruction|Description|
|-|-|
|mtcr|Move to Control Register|
|mfcr|Move from Control Register|
|mscr|Swap General Purpose Register with Control Register|
|syscall|System call|
|vret|Vector return|
|rdtick|Read tick counter|
|dbg|Debug break. One byte opcode.|
|cpufeat|CPU features|
|eint|Enable interrupts|
|dint|Disable interrupts|
|tlbflush|Flushes one page, not needed if process identifiers are used. Switching page table with 0 as process identifier flushes whole page table|
|slow|Indicate spinlock to the CPU|
|wfi|Wait for interrupt|


# Known Issues

In an exception handler you want to save registers somewhere. To the stack or to a kernel block of memory or in a process context memory block. To do so you must load pointers and use registers. But you can't use registers because you are trying to save them.
You can push to a global variable but that doesn't work with multiple threads. You can get CPUID and index into an array of temp data per CORE but you need to move CPUID to general purpose register so that's not possible.

The solution at the moment is adding CRKERNEL which the kernel can do whatever it wants with. The kernel can reserve it for use in exception handlers to move GPRs to. It can store CORE context pointer in it or whatever. We implement `mscr` to swap general purpose and control registers.

```arm


#define CONTEXT_SIZE 512
core_context:
    byte[32*#CONTEXT_SIZE]

timer_hits:
    long 0

ex_handler:
    
    mtcr r0, CRKERNEL
    mfcr r0, CRCPUID

    umul r0, r0, #CONTEXT_SIZE
    lea r0, [core_context + r0]

    sth r1, [r0 + 2]
    
    ldh r1, [timer_hits]
    add r1, r1, 1
    sth r1, [timer_hits]

    mfcr r0, CRKERNEL
    vret

ex_handler:
    mscr sp, CRKERNEL

    push r1

    ldh r1, [timer_hits]
    add r1, r1, 1
    sth r1, [timer_hits]

    pop r1

    mscr sp, CRKERNEL

    vret

```

Alternatively we implement CREXSTACK and CRESP which does `CRESP = SP; SP = CREXSTACK` on exceptions. `vret` would `SP = CRESP`. It could push the stack pointer onto the exception stack if you don't want CRESP as extra control register.

```arm
    byte[512]
exstack:

init_exstack:
    // Prepare exception stack
    lea r0, [exstack]
    mtcr r0, CREXSTACK
    ret

ex_handler:
    push r1
    
    ldh r1, [timer_hits]
    add r1, r1, 1
    sth r1, [timer_hits]

    pop r1
    vret
```
