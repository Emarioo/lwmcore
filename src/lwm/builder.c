
#include "lwm/builder.h"

#include "lwm/util.h"

#include <stdlib.h>
#include <string.h>


void builder_init(Builder* builder) {
    builder->byteStream_len = 0;
    builder->byteStream_max = 0x100000;
    builder->byteStream = malloc(builder->byteStream_max);
}

void builder_init_stream(Builder* builder, void* ptr, size_t head, size_t max) {
    builder->byteStream_len = head;
    builder->byteStream_max = max;
    builder->byteStream = ptr;
}



#define APPEND_BYTES(bytes)                                         \
    Assert(builder->byteStream_len + ARRAY_LENGTH(bytes) <= builder->byteStream_max);  \
    for (int bi=0;bi<ARRAY_LENGTH(bytes);bi++) {                    \
        builder->byteStream[builder->byteStream_len] = bytes[bi];   \
        builder->byteStream_len++;                                  \
    }

void emit_li8(Builder* builder, int reg0, uint8_t imm) {
    uint8_t bytes[] = {
        OPCODE_LI8,
        reg0,
        imm,
    };
    APPEND_BYTES(bytes);
}
void emit_li16(Builder* builder, int reg0, uint16_t imm) {
    uint8_t bytes[] = {
        OPCODE_LI16,
        reg0,
        (imm >> 0) & 0xFF,
        (imm >> 8) & 0xFF,
    };
    APPEND_BYTES(bytes);
}
void emit_li32(Builder* builder, int reg0, uint32_t imm) {
    uint8_t bytes[] = {
        OPCODE_LI32,
        reg0,
        (imm >>  0) & 0xFF,
        (imm >>  8) & 0xFF,
        (imm >> 16) & 0xFF,
        (imm >> 24) & 0xFF,
    };
    APPEND_BYTES(bytes);
}
void emit_li64(Builder* builder, int reg0, uint64_t imm) {
    uint8_t bytes[] = {
        OPCODE_LI64,
        reg0,
        (imm >>  0) & 0xFF,
        (imm >>  8) & 0xFF,
        (imm >> 16) & 0xFF,
        (imm >> 24) & 0xFF,
        (imm >> 32) & 0xFF,
        (imm >> 40) & 0xFF,
        (imm >> 48) & 0xFF,
        (imm >> 56) & 0xFF,
    };
    APPEND_BYTES(bytes);
}

#define EMIT_REG1(opcode) \
    uint8_t bytes[] = {   \
        opcode,           \
        reg0,             \
    };                    \
    APPEND_BYTES(bytes)
#define EMIT_REG2(opcode) \
    uint8_t bytes[] = {   \
        opcode,           \
        reg0,             \
        reg1,             \
    };                    \
    APPEND_BYTES(bytes)
#define EMIT_REG3(opcode) \
    uint8_t bytes[] = {   \
        opcode,           \
        reg0,             \
        reg1,             \
        reg2,             \
    };                    \
    APPEND_BYTES(bytes)
#define EMIT_REG4(opcode) \
    uint8_t bytes[] = {   \
        opcode,           \
        reg0,             \
        reg1,             \
        reg2,             \
        reg3,             \
    };                    \
    APPEND_BYTES(bytes)
#define EMIT_VOID(opcode) \
    uint8_t bytes[] = {   \
        opcode,           \
    };                    \
    APPEND_BYTES(bytes)

void emit_call_reg(Builder* builder, int reg0) {
    EMIT_REG1(OPCODE_CALL_REG);
}
void emit_jmp_reg(Builder* builder, int reg0) {
    EMIT_REG1(OPCODE_JMP_REG);
}
void emit_push(Builder* builder, int reg0) {
    EMIT_REG1(OPCODE_PUSH);
}
void emit_pop(Builder* builder, int reg0) {
    EMIT_REG1(OPCODE_POP);
}
void emit_tlbflush(Builder* builder, int reg0) {
    EMIT_REG1(OPCODE_TLBFLUSH);
}

void emit_save(Builder* builder, int reg0, uint8_t imm8) {
    uint8_t bytes[] = {
        OPCODE_SAVE,
        reg0,
        imm8,
    };
    APPEND_BYTES(bytes);
}
void emit_restore(Builder* builder, int reg0, uint8_t imm8) {
    uint8_t bytes[] = {
        OPCODE_RESTORE,
        reg0,
        imm8,
    };
    APPEND_BYTES(bytes);
}
void emit_rdtick(Builder* builder, int reg0) {
    EMIT_REG1(OPCODE_RDTICK);
}
void emit_rdtick1(Builder* builder, int reg0, int reg1) {
    EMIT_REG1(OPCODE_RDTICK1);
}
void emit_rdtick2(Builder* builder, int reg0, int reg1, int reg2, int reg3) {
    EMIT_REG1(OPCODE_RDTICK2);
}

void emit_not(Builder* builder, int reg0, int reg1) {
    EMIT_REG2(OPCODE_NOT);
}
void emit_mfcr(Builder* builder, int reg0, int reg1) {
    EMIT_REG2(OPCODE_MFCR);
}
void emit_mtcr(Builder* builder, int reg0, int reg1) {
    EMIT_REG2(OPCODE_MTCR);
}
void emit_mscr(Builder* builder, int reg0, int reg1) {
    EMIT_REG2(OPCODE_MSCR);
}
void emit_cpufeat(Builder* builder, int reg0, int reg1) {
    EMIT_REG2(OPCODE_CPUFEAT);
}

void emit_add(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_ADD);
}
void emit_sub(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_SUB);
}
void emit_umul(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_UMUL);
}
void emit_smul(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_SMUL);
}
void emit_udiv(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_UDIV);
}
void emit_sdiv(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_SDIV);
}
void emit_umod(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_UMOD);
}
void emit_smod(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_SMOD);
}
void emit_and(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_AND);
}
void emit_or(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_OR);
}
void emit_xor(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_XOR);
}
void emit_shl(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_SHL);
}
void emit_shr(Builder* builder, int reg0, int reg1, int reg2) {
    EMIT_REG3(OPCODE_SHR);
}


void emit_ret(Builder* builder) {
    EMIT_VOID(OPCODE_RET);
}
void emit_syscall(Builder* builder) {
    EMIT_VOID(OPCODE_SYSCALL);
}
void emit_vret(Builder* builder) {
    EMIT_VOID(OPCODE_VRET);
}
void emit_dbg(Builder* builder) {
    EMIT_VOID(OPCODE_DBG);
}
void emit_eint(Builder* builder) {
    EMIT_VOID(OPCODE_EINT);
}
void emit_dint(Builder* builder) {
    EMIT_VOID(OPCODE_DINT);
}
void emit_slow(Builder* builder) {
    EMIT_VOID(OPCODE_SLOW);
}
void emit_wfi(Builder* builder) {
    EMIT_VOID(OPCODE_WFI);
}
void emit_nop(Builder* builder) {
    EMIT_VOID(OPCODE_NOP);
}
void emit_vmstart(Builder* builder) {
    EMIT_VOID(OPCODE_VMSTART);
}


static inline int memop_to_opcode(MemoryInstructionKind kind) {
    return OPCODE_LEA + kind - MEMOP_LEA;
}

void emit_memop(Builder* builder, MemoryInstructionKind kind, AddressingForm form, int reg0, int reg1, int reg_base, int reg_index, int64_t in_displacement, uint64_t* fixup) {
    int opcode = memop_to_opcode(kind);

    int64_t displacement = in_displacement;

    if (kind != MEMOP_CAS) {
        uint8_t bytes[] = {
            opcode,
            (reg0 << 3) | ADDRESSING_MASK_PRI(form),
        };
        APPEND_BYTES(bytes);
    } else {
        uint8_t bytes[] = {
            opcode,
            (reg0 << 3) | ADDRESSING_MASK_PRI(form),
            reg1,
        };
        APPEND_BYTES(bytes);
    }

    switch (form) {
        case ADDRESSING_ABS16: {
            uint8_t bytes[] = {
                displacement & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_ABS32: {
            uint8_t bytes[] = {
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_ABS64: {
            uint8_t bytes[] = {
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
                (displacement >> 32) & 0xFF,
                (displacement >> 40) & 0xFF,
                (displacement >> 48) & 0xFF,
                (displacement >> 56) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_DISP8: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_DISP16: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_DISP32: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_DISP64: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
                (displacement >> 32) & 0xFF,
                (displacement >> 40) & 0xFF,
                (displacement >> 48) & 0xFF,
                (displacement >> 56) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_PC_DISP8: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
            };
            APPEND_BYTES(bytes);
            *fixup = builder->byteStream_len - 1;
        } break;
        case ADDRESSING_REG1_PC_DISP16: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
            *fixup = builder->byteStream_len - 2;
        } break;
        case ADDRESSING_REG1_PC_DISP32: {
            uint8_t bytes[] = {
                (reg_base << 3) | ADDRESSING_MASK_SEC(form),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
            *fixup = builder->byteStream_len - 4;
        } break;
        case ADDRESSING_REG2_DISP8: {
            uint8_t bytes[] = {
                (reg_base << 3) | (ADDRESSING_MASK_SEC(form) & 0x7),
                (reg_index << 3) | ((ADDRESSING_MASK_SEC(form) >> 3) & 0x7),
                (displacement >> 0) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG2_DISP16: {
            uint8_t bytes[] = {
                (reg_base << 3) | (ADDRESSING_MASK_SEC(form) & 0x7),
                (reg_index << 3) | ((ADDRESSING_MASK_SEC(form) >> 3) & 0x7),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG2_DISP32: {
            uint8_t bytes[] = {
                (reg_base << 3) | (ADDRESSING_MASK_SEC(form) & 0x7),
                (reg_index << 3) | ((ADDRESSING_MASK_SEC(form) >> 3) & 0x7),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_PC_DISP8: {
            uint8_t bytes[] = {
                (displacement >> 0) & 0xFF,
            };
            APPEND_BYTES(bytes);
            *fixup = builder->byteStream_len - 1;
        } break;
        case ADDRESSING_PC_DISP16: {
            uint8_t bytes[] = {
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
            *fixup = builder->byteStream_len - 2;
        } break;
        case ADDRESSING_PC_DISP32: {
            uint8_t bytes[] = {
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
            *fixup = builder->byteStream_len - 4;
        } break;
        default:
            Assert(false);
    }
}


void emit_jmp8(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JMP,
        0,
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_jmp16(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JMP1,
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 2;
}
void emit_jmp32(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JMP2,
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 4;
}
void emit_call8(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_CALL,
        0,
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_call16(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_CALL1,
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 2;
}
void emit_call32(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_CALL2,
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 4;
}


void emit_jz8(Builder* builder, int reg0, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JZ,
        (reg0 << 3) | 0,
        0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_jz16(Builder* builder, int reg0, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JZ,
        (reg0 << 3) | 1,
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 2;
}
void emit_jz32(Builder* builder, int reg0, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JZ,
        (reg0 << 3) | 2,
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 4;
}

void emit_jnz8(Builder* builder, int reg0, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JNZ,
        (reg0 << 3) | 0,
        0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_jnz16(Builder* builder, int reg0, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JNZ,
        (reg0 << 3) | 1,
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 2;
}
void emit_jnz32(Builder* builder, int reg0, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JNZ,
        (reg0 << 3) | 2,
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 4;
}

void emit_jcond8(Builder* builder, ConditionKind kind, int reg0, int reg1, uint64_t* fixup) {
    int relsize = 0;
    uint8_t bytes[] = {
        OPCODE_JCOND,
        relsize | (kind << 2) | ((reg0&3) << 6),
        (reg0 >> 2) | (reg1 << 3),
        0,
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_jcond16(Builder* builder, ConditionKind kind, int reg0, int reg1, uint64_t* fixup) {
    int relsize = 1;
    uint8_t bytes[] = {
        OPCODE_JCOND,
        relsize | (kind << 2) | ((reg0&3) << 6),
        (reg0 >> 2) | (reg1 << 3),
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 2;
}
void emit_jcond32(Builder* builder, ConditionKind kind, int reg0, int reg1, uint64_t* fixup) {
    int relsize = 2;
    uint8_t bytes[] = {
        OPCODE_JCOND,
        relsize | (kind << 2) | ((reg0&3) << 6),
        (reg0 >> 2) | (reg1 << 3),
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 4;
}