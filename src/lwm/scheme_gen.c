#include "lwm/scheme_gen.h"

#include "lwm/parser_util.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


#define ERROR_SRC_RET(HEAD, FORMAT, ...) SourceLocation loc = get_location(context, HEAD); error_src(loc, FORMAT, ##__VA_ARGS__); longjmp(context->jumpBuffer, 1);

void parse_scheme(const char* path, scheme_Database* database) {
    bool res;
    char*  fileBuffer;
    size_t fileBuffer_len;
    res = readFile(path, (void**)&fileBuffer, &fileBuffer_len);
    if (!res) {
        return;
    }

    ParserContext _context = {0};
    ParserContext* context = &_context;

    string raw_text = { .len = fileBuffer_len,
                        .ptr = fileBuffer };
    string text;

    res = preprocess_text(context, raw_text, &text, path);
    if (!res) {
        return;
    }

    // writeFile("scheme.pp", text.ptr, text.len);


    context->path = path;
    context->text = text.ptr;
    context->text_len = text.len;

    int jmpResult = setjmp(context->jumpBuffer);
    if (jmpResult != 0) {
        // Error message has been printed
        return;
    }

    memset(database, 0, sizeof(*database));

    database->fields_len = 0;
    database->fields = malloc(sizeof(scheme_Field) * 50);
    
    database->priorities_len = 0;
    database->priorities = malloc(sizeof(scheme_Priority) * 40);
    
    database->instructions_len = 0;
    database->instructions = malloc(sizeof(scheme_Instruction) * 1024);

    int head = 0;
    int parsedChars = 0;

    while (true) {

        parse_space(context, &head);

        if (parse_eof(context, head)) {
            break;
        }

        string word;
        parsedChars = parse_name(context, &head, &word);
        if (!parsedChars) {
            ERROR_SRC_RET(head, "Expected a keyword\n");
        }

        if (equal(word, "field")) {
            
            parse_space(context, &head);

            string name;
            parsedChars = parse_name(context, &head, &name);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a field name\n");
            }

            parse_space(context, &head);

            uint64_t bitCount;
            parsedChars = parse_int(context, &head, &bitCount);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a bit count\n");
            }

            scheme_Field* prevField = find_field(database, name.ptr);
            if (prevField) {
                ERROR_SRC_RET(head, "Field name must be unique.\n");
            }

            scheme_Field* field = &database->fields[database->fields_len];
            database->fields_len++;

            strcpy(field->name, name.ptr);
            field->bits = bitCount;

        } else if (equal(word, "priority")) {
            
            parse_space(context, &head);

            string name;
            parsedChars = parse_name(context, &head, &name);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a priority name.\n");
            }

            parse_space(context, &head);

            uint64_t prioValue;
            parsedChars = parse_int(context, &head, &prioValue);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a priority value.\n");
            }

            scheme_Priority* prevPrio = find_priority(database, name.ptr);
            if (prevPrio) {
                ERROR_SRC_RET(head, "Priority name must be unique.\n");
            }

            scheme_Priority* priority = &database->priorities[database->priorities_len];
            database->priorities_len++;

            strcpy(priority->name, name.ptr);
            priority->priority = prioValue;

        } else if (equal(word, "instruction")) {
               
            parse_space(context, &head);

            string name;
            parsedChars = parse_name(context, &head, &name);
            if (!parsedChars) {
                ERROR_SRC_RET(head, "Expected a priority name.\n");
            }

            scheme_Instruction* prevInst = find_instruction(database, name.ptr);
            if (prevInst) {
                ERROR_SRC_RET(head, "Instruction name must be unique.\n");
            }

            scheme_Instruction* inst = &database->instructions[database->instructions_len];
            database->instructions_len++;

            strcpy(inst->name, name.ptr);
            inst->fields_len = 0;
            inst->priority = NULL;
            inst->encoding = NULL;

            while (true) {

                parse_space_not_newline(context, &head);

                char chr = get_char(context, head);
                if (chr == '\n') {
                    break;
                }

                string fieldName;
                parsedChars = parse_name(context, &head, &fieldName);
                if (!parsedChars) {
                    ERROR_SRC_RET(head, "Expected a field name.\n");
                }

                scheme_Field* field = find_field(database, fieldName.ptr);
                if (field) {
                    inst->fields[inst->fields_len] = field;
                    inst->fields_len++;
                } else {
                    scheme_Priority* prio = find_priority(database, fieldName.ptr);
                    if (!prio) {
                        ERROR_SRC_RET(head, "'%s' is not defined field or priority.\n", fieldName.ptr);
                    }
                    if (inst->priority) {
                        ERROR_SRC_RET(head, "Cannot define another priority on instruction '%s' (prio=%s).\n", name.ptr, fieldName.ptr);
                    }
                    inst->priority = prio;
                }
            }
        } else {
            ERROR_SRC_RET(head, "Expected a field, priority, or instruction.\n");
        }
    }
    
    generate_encoder(database);

    dump_scheme(database);
}


void apply_encoding(scheme_Instruction* inst, int opcodeID) {
    memset(inst->encoding->fixedBits, 0, sizeof(inst->encoding->fixedBits));

    int neededOpcodeBits;
    if (opcodeID == 0)
        neededOpcodeBits = 1;
    else
        neededOpcodeBits = 32 - lzcnt(opcodeID+1);
    inst->encoding->numOpcodeBits = neededOpcodeBits;

    for (int i = 0; i < neededOpcodeBits; i++) {
        int byte = i >> 3;
        int bit  = i & 7;

        if ((opcodeID >> i) & 1) 
            inst->encoding->fixedBits[byte] |= (1 << bit);
    }

    int totalBits = inst->encoding->numOpcodeBits + inst->encoding->numNonOpcodeBits;
    int paddedBits = (totalBits + 7) & ~7;
    inst->encoding->numUnusedBits = paddedBits - totalBits;
}

int cmp_priority(const void* a, const void* b) {
    scheme_Priority* pa = *(scheme_Priority**)a;
    scheme_Priority* pb = *(scheme_Priority**)b;
    if (pa == NULL)
        return 1;
    if (pb == NULL)
        return -1;

    return pb->priority - pa->priority; // descending
}

void generate_encoder(scheme_Database* db) {

    /*
        Stuff to think about:

            The opcode is a decision tree.


        We score unused bits and number of opcode bits. Low overall score means compact.
    */

    int encodings_len = 0;
    scheme_Encoding* encodings = malloc(sizeof(scheme_Encoding) * 1000);

    int instructions_len = db->instructions_len;
    scheme_Instruction* instructions = db->instructions;


    int sortedPriorities_len = db->priorities_len+1;
    scheme_Priority** sortedPriorities = malloc(sizeof(scheme_Priority*) * sortedPriorities_len);
    for (int i = 0; i < sortedPriorities_len - 1; i++) {
        sortedPriorities[i] = &db->priorities[i];
    }
    sortedPriorities[sortedPriorities_len-1] = NULL; // Last one is NULL for the default ones.
    qsort(sortedPriorities, sortedPriorities_len, sizeof(scheme_Priority*), cmp_priority);



    int nextOpcodeID = 0;

    for (int pi=0;pi<sortedPriorities_len;pi++) {
        scheme_Priority* prio = sortedPriorities[pi];
        for (int si=0;si<instructions_len;si++) {
            scheme_Instruction* inst = &instructions[si];

            if (inst->priority != prio)
                continue;

            inst->encoding = &encodings[encodings_len];
            encodings_len++;
            memset(inst->encoding, 0, sizeof(scheme_Encoding));

            for (int fi=0;fi<inst->fields_len;fi++) {
                scheme_Field* field = inst->fields[fi];
                inst->encoding->numNonOpcodeBits += field->bits;
                // printf("Fbits %d\n", inst->encoding->numNonOpcodeBits);
            }

            apply_encoding(inst, nextOpcodeID);
            nextOpcodeID++;
        }
    }

    // Score

    int numOpcodeBits = 0;
    int numUnusedBits = 0;
    
    for (int si=0;si<instructions_len;si++) {
        scheme_Instruction* inst = &instructions[si];

        numOpcodeBits += inst->encoding->numOpcodeBits;
        numUnusedBits += inst->encoding->numUnusedBits;
    }

    printf("Total opcode bits: %d (avg %.2f)\n", numOpcodeBits, (float)numOpcodeBits/(float)instructions_len);
    printf("Total unused bits: %d (avg %.2f)\n", numUnusedBits, (float)numUnusedBits/(float)instructions_len);
}

void dump_scheme(scheme_Database* db) {
    printf("Fields[%d]\n", db->fields_len);
    for (int i=0;i<db->fields_len;i++) {
        printf(" %s %d\n", db->fields[i].name, db->fields[i].bits);
    }
    printf("Priorities[%d]\n", db->priorities_len);
    for (int i=0;i<db->priorities_len;i++) {
        printf(" %s %d\n", db->priorities[i].name, db->priorities[i].priority);
    }
    printf("Instructions[%d]\n", db->instructions_len);
    for (int i=0;i<db->instructions_len;i++) {
        scheme_Instruction* inst = &db->instructions[i];
        printf(" %s ", inst->name);
        for (int fi=0;fi<inst->fields_len;fi++) {
            printf(" %s", inst->fields[fi]->name);
        }
        if (inst->priority) {
            printf(" prio=%s", inst->priority->name);
        }
        printf("\n");
        printf("  ");
        if (inst->encoding) {
            printf(" %d", inst->encoding->numOpcodeBits);
            printf(" %d ", inst->encoding->numNonOpcodeBits);
            int totalBits = inst->encoding->numOpcodeBits + inst->encoding->numNonOpcodeBits + inst->encoding->numUnusedBits;
            for (int bi=0;bi<totalBits;bi++) {
                if (bi < inst->encoding->numOpcodeBits) {
                    int bit = (inst->encoding->fixedBits[bi/8] >> (bi&7)) & 1;
                    if (bit)
                        printf("1");
                    else
                        printf("0");
                } else if (bi < inst->encoding->numOpcodeBits + inst->encoding->numNonOpcodeBits) {
                    printf("f");
                } else {
                    printf("u");
                }
            }
            printf("\n");
        }
    }
}


scheme_Field* find_field(scheme_Database* db, const char* name) {
    for (int i = 0; i < db->fields_len; i++)
        if (!strcmp(db->fields[i].name, name))
            return &db->fields[i];

    return NULL;
}

scheme_Priority* find_priority(scheme_Database* db, const char* name) {
    for (int i = 0; i < db->priorities_len; i++)
        if (!strcmp(db->priorities[i].name, name))
            return &db->priorities[i];

    return NULL;
}

scheme_Instruction* find_instruction(scheme_Database* db, const char* name) {
    for (int i = 0; i < db->instructions_len; i++)
        if (!strcmp(db->instructions[i].name, name))
            return &db->instructions[i];

    return NULL;
}
