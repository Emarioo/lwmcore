
This document specifies the Application Binary Interface (ABI) used by tests, examples, and other programs in this project.

The CPU architecture does not enforce this ABI but we must still have one for tests, etc.

# Calling Convention

Arguments: r0 - r5

Return value: r0, r1

Volatile registers: r0 - r5

Non-volatile registers: r6 - r15, r16 - r31.
Note that sp,lr,tp are r15,r14,r13 and non-volatile.

Control registers cannot be modified by user programs.


