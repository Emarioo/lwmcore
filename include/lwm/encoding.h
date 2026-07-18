
#pragma once

#include "lwm/asm_types.h"
#include "lwm/builder.h"

bool prepare_encode_inst(string mnemonic, Instruction* inst, int* minBytes, int* maxBytes);

bool encode_inst(Builder* builder, Instruction* inst, char message[512]);

#define INVALID_REGISTER -1
int regname_to_number(string name);
