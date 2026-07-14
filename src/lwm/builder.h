
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "lwm/isa.h"

typedef struct {
    uint8_t* byteStream;
    size_t   byteStream_len;
    size_t   byteStream_max;
} Builder;

void builder_init(Builder* builder);
void builder_init_stream(Builder* builder, void* ptr, size_t head, size_t max);
static inline int builder_head(Builder* builder) {
    return builder->byteStream_len;
}
static inline void emit_zeros(Builder* builder, int size) {
    for (int i=0;i<size;i++) {
        builder->byteStream[builder->byteStream_len] = 0;
        builder->byteStream_len++;
    }
}

static inline void emit_bytes(Builder* builder, uint8_t* data, int size) {
    for (int i=0;i<size;i++) {
        builder->byteStream[builder->byteStream_len] = data[i];
        builder->byteStream_len++;
    }
}

void emit_li8(Builder* builder, int reg0, uint8_t imm);
void emit_li16(Builder* builder, int reg1, uint16_t imm);
void emit_li32(Builder* builder, int reg2, uint32_t imm);
void emit_li64(Builder* builder, int reg3, uint64_t imm);
void emit_lis8(Builder* builder, int reg0, uint8_t imm);
void emit_lis16(Builder* builder, int reg1, uint16_t imm);
void emit_lis32(Builder* builder, int reg2, uint32_t imm);
void emit_lis64(Builder* builder, int reg3, uint64_t imm);

void emit_call_reg(Builder* builder, int reg0);
void emit_jmp_reg(Builder* builder, int reg0);
void emit_push(Builder* builder, int reg0);
void emit_pop(Builder* builder, int reg0);
void emit_tlbflush(Builder* builder, int reg0);

void emit_not(Builder* builder, int reg0, int reg1);
void emit_mfcr(Builder* builder, int reg0, int reg1);
void emit_mtcr(Builder* builder, int reg0, int reg1);
void emit_mscr(Builder* builder, int reg0, int reg1);
void emit_cpufeat(Builder* builder, int reg0, int reg1);

void emit_add(Builder* builder, int reg0, int reg1, int reg2);
void emit_sub(Builder* builder, int reg0, int reg1, int reg2);
void emit_umul(Builder* builder, int reg0, int reg1, int reg2);
void emit_smul(Builder* builder, int reg0, int reg1, int reg2);
void emit_udiv(Builder* builder, int reg0, int reg1, int reg2);
void emit_sdiv(Builder* builder, int reg0, int reg1, int reg2);
void emit_umod(Builder* builder, int reg0, int reg1, int reg2);
void emit_smod(Builder* builder, int reg0, int reg1, int reg2);
void emit_and(Builder* builder, int reg0, int reg1, int reg2);
void emit_or(Builder* builder, int reg0, int reg1, int reg2);
void emit_xor(Builder* builder, int reg0, int reg1, int reg2);
void emit_shl(Builder* builder, int reg0, int reg1, int reg2);
void emit_shr(Builder* builder, int reg0, int reg1, int reg2);


void emit_ret(Builder* builder);
void emit_syscall(Builder* builder);
void emit_vret(Builder* builder);
void emit_dbg(Builder* builder);
void emit_eint(Builder* builder);
void emit_dint(Builder* builder);
void emit_slow(Builder* builder);
void emit_wfi(Builder* builder);
void emit_nop(Builder* builder);
void emit_vmstart(Builder* builder);

void emit_save(Builder* builder, int reg0, uint8_t imm8);
void emit_restore(Builder* builder, int reg0, uint8_t imm8);

typedef enum {
    MEMOP_LEA,
    MEMOP_LDB,
    MEMOP_LDBS,
    MEMOP_LDH,
    MEMOP_LDHS,
    MEMOP_LDL,
    MEMOP_LDLS,
    MEMOP_LDQ,
    MEMOP_STB,
    MEMOP_STH,
    MEMOP_STL,
    MEMOP_STQ,
    MEMOP_XADD,

    MEMOP_CAS,
} MemoryInstructionKind;



void emit_rdtick(Builder* builder, int reg0);
void emit_rdtick1(Builder* builder, int reg0, int reg1);
void emit_rdtick2(Builder* builder, int reg0, int reg1, int reg2, int reg3);



void emit_memop(Builder* builder, MemoryInstructionKind kind, AddressingForm form, int reg0, int reg1, int reg_base, int reg_index, int64_t displacement, uint64_t* fixup);

void emit_jmp32(Builder* builder, uint64_t* fixup);
void emit_jmp16(Builder* builder, uint64_t* fixup);
void emit_jmp8(Builder* builder, uint64_t* fixup);

void emit_call32(Builder* builder, uint64_t* fixup);
void emit_call16(Builder* builder, uint64_t* fixup);
void emit_call8(Builder* builder, uint64_t* fixup);

void emit_jz8(Builder* builder, int reg0, uint64_t* fixup);
void emit_jz16(Builder* builder, int reg0, uint64_t* fixup);
void emit_jz32(Builder* builder, int reg0, uint64_t* fixup);

void emit_jnz8(Builder* builder, int reg0, uint64_t* fixup);
void emit_jnz16(Builder* builder, int reg0, uint64_t* fixup);
void emit_jnz32(Builder* builder, int reg0, uint64_t* fixup);

void emit_jcond8(Builder* builder, ConditionKind kind, int reg0, int reg1, uint64_t* fixup);
void emit_jcond16(Builder* builder, ConditionKind kind, int reg0, int reg1, uint64_t* fixup);
void emit_jcond32(Builder* builder, ConditionKind kind, int reg0, int reg1, uint64_t* fixup);

