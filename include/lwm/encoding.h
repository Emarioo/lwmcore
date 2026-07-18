
#pragma once

#include "lwm/builder.h"


typedef struct CoreState CoreState;

typedef void(*FN_mmu_operation)(CoreState* core, MMUOperation operation, uint64_t in_address, uint64_t size, void* data, bool physical);

typedef struct {
    FN_mmu_operation mmu_operation;
    CoreState* core;
} Decoder;

bool prepare_encode_inst(string mnemonic, Instruction* inst, int* minBytes, int* maxBytes);

bool encode_inst(Builder* builder, Instruction* inst, char message[512]);

int decode_inst(Decoder* decoder, uintptr_t pc, uint32_t* opcode, uint8_t* regs, uint64_t* immediate, ConditionKind* cond, AddressingForm* addressingForm);

#define INVALID_REGISTER -1
int regname_to_number(string name);
