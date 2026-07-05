
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
    for (int bi=0;bi<sizeof(bytes)/sizeof(*bytes);bi++) {           \
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

void emit_memop(Builder* builder, MemoryInstructionKind kind, AddressingForm form, int reg0, int reg_base, int reg_index, int64_t in_displacement) {
    int opcode = memop_to_opcode(kind);

    int64_t displacement = in_displacement;

    switch (form) {
        case ADDRESSING_ABS16: {
            int flags = 0;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                displacement & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_ABS32: {
            int flags = 1;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_ABS64: {
            int flags = 2;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
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
            int flags = 3;
            int extraFlags = 0;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (reg_base << 3) | extraFlags,
                (displacement >> 0) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_DISP16: {
            int flags = 3;
            int extraFlags = 1;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (reg_base << 3) | extraFlags,
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG1_DISP32: {
            int flags = 3;
            int extraFlags = 2;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (reg_base << 3) | extraFlags,
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG2_DISP8: {
            int flags = 4;
            int extraFlags = 0;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (reg_base << 3) | (extraFlags & 0x7),
                (reg_index << 3) | ((extraFlags >> 3) & 0x7),
                (displacement >> 0) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG2_DISP16: {
            int flags = 4;
            int extraFlags = 1;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (reg_base << 3) | (extraFlags & 0x7),
                (reg_index << 3) | ((extraFlags >> 3) & 0x7),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_REG2_DISP32: {
            int flags = 4;
            int extraFlags = 2;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (reg_base << 3) | (extraFlags & 0x7),
                (reg_index << 3) | ((extraFlags >> 3) & 0x7),
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
        } break;
        case ADDRESSING_PC_DISP32: {
            int flags = 5;
            uint8_t bytes[] = {
                opcode,
                (reg0 << 3) | flags,
                (displacement >> 0) & 0xFF,
                (displacement >> 8) & 0xFF,
                (displacement >> 16) & 0xFF,
                (displacement >> 24) & 0xFF,
            };
            APPEND_BYTES(bytes);
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
        OPCODE_JMP,
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_jmp32(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_JMP,
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
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
        OPCODE_CALL,
        0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
void emit_call32(Builder* builder, uint64_t* fixup) {
    uint8_t bytes[] = {
        OPCODE_CALL,
        0, 0, 0, 0
    };
    APPEND_BYTES(bytes)
    *fixup = builder->byteStream_len - 1;
}
// void emit_jz(Builder* builder, int reg0, int32_t** relative);
// void emit_jnz(Builder* builder, int reg0, int32_t** relative);

// void emit_jcond(Builder* builder, ConditionKind kind, int reg0, int reg1, int32_t** relative);
