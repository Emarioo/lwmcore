
#include "lwmcore/assembler.h"
#include "lwmcore/emulator.h"
#include "lwmcore/blotter.h"

void print_help() {
    printf("Usage: lwm sample.asm -o sample.bin\n");
    printf("  -o <path>       : Path where to place output binary file.\n");
    printf("  -e,--emulate    : Assembles and emulates the file.\n");
    printf("  -r,--rom <path> : Assembles and writes a Logic World subassembly file.\n");
    printf("  -f,--force      : Will overwrite user made subassembly.\n");
    printf("  --safe          : Don't write any files, log which ones would have been written to.\n");
}

int main(int argc, const char** argv) {
    string assembly_file = {0};
    string bin_file = {0, nullptr};
    string rom_file = {0, nullptr};
    bool do_emulate = false;
    bool do_rom = false;
    bool overwrite_user_subassembly = false;

    for(int i=1;i<argc;i++) {
        const char* arg = argv[i];
        if(!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            print_help();
            return 0;
        } else if(!strcmp(arg, "-v") || !strcmp(arg, "--version")) {
            // TODO: Remove this at some point?
            time_t now = time(nullptr);
            srand(now);
            float num = (float)rand() / (float)RAND_MAX;
            if(num < 0.2f) {
                // 20%
                printf("Hello, no version for you this time.\n");
            } else if(num < 0.5f) {
                // 30%
                printf("Version is apparently v0.0.1\n");
            } else if(num < 0.95f) {
                // 45%
                printf("Hmm... I'll tell you the first two numbers: v0.0.?\n");
            } else {
                // 5%
                printf("The version is v0.0.1 (2025-03-30)\n");
                printf("(you have a 5%% chance of seeing this message)\n");
            }
            return 0;
        } else if(!strcmp(arg, "-e") || !strcmp(arg, "--emulate")) {
            do_emulate = true;
        } else if(!strcmp(arg, "-f") || !strcmp(arg, "--force")) {
            overwrite_user_subassembly = true;
        } else if(!strcmp(arg, "--safe")) {
            DISABLE_FILE_WRITES = true;
        } else if(!strcmp(arg, "-r") || !strcmp(arg, "--rom")) {
            if(i+1 >= argc) {
                error("Missing argument after '%s'\n", arg);
                return 1;
            }
            do_rom = true;
            arg = argv[i+1];
            i++;
            rom_file.len = strlen(arg);
            rom_file.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        } else if(!strcmp(arg, "-o")) {
            if(i+1 >= argc) {
                error("Missing argument after '%s'\n", arg);
                return 1;
            }
            arg = argv[i+1];
            i++;
            bin_file.len = strlen(arg);
            bin_file.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        } else {
            if(assembly_file.ptr) {
                error("You cannot specify a second file '%s'. ('%s' was first)\n", arg, assembly_file.ptr);
                return 1;
            }
            assembly_file.len = strlen(arg);
            assembly_file.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        }
    }

    if(assembly_file.len == 0) {
        print_help();
        return 0;
    }

    #define IS_SUBASSEMBLY(P) endswith(P, ".logicworld") || endswith(P, ".partialworld") || endswith(P, ".lwsubassembly")

    bool file_is_asm = endswith(assembly_file, ".asm");
    bool file_is_bin = endswith(assembly_file, ".bin");
    bool file_is_subassembly = IS_SUBASSEMBLY(assembly_file);

    if (!file_is_asm && !file_is_bin && !file_is_subassembly) {
        error("File must be .asm to assemble or .bin to emulate or .logicworld/.partialworld to deserialize. Not '%s'.\n", assembly_file.ptr);
        return 1;
    }

    Bin bin;
    if(file_is_subassembly) {
        BlotterData* data = decompose_blotter_file(assembly_file);
        printf("(no crash)\n");
        return 0; // we only want to deserialize .logicworld, we can't emulate or generate binary
    } else if (file_is_asm) {
        FILE* file = fopen(assembly_file.ptr, "rb");
        if (!file) {
            error("Could not read '%s'\n", assembly_file.ptr);
            return 1;
        }
        string buffer;
        fseek(file, 0, SEEK_END);
        buffer.len = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer.ptr = (char*)malloc(buffer.len);
        fread(buffer.ptr, 1, buffer.len, file);
        fclose(file);
        
        AssemblerInfo* info = (AssemblerInfo*)malloc(sizeof(AssemblerInfo));
        memset(info, 0, sizeof(*info));
        info->source_file = assembly_file;
        bool res = assemble(buffer, &bin, info);
        if(!res)
            return 1;

        dump(bin);
    } else if(file_is_bin) {
        FILE* file = fopen(assembly_file.ptr, "rb");
        if (!file) {
            error("Could not read '%s'\n", assembly_file.ptr);
            return 1;
        }
        string buffer;
        fseek(file, 0, SEEK_END);
        buffer.len = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer.ptr = (char*)malloc(buffer.len);
        fread(buffer.ptr, 1, buffer.len, file);
        fclose(file);

        bin.len = buffer.len;
        bin.max = buffer.len;
        bin.ptr = buffer.ptr;
    }

    if (do_emulate) {
        EmulatorInfo* info = (EmulatorInfo*)malloc(sizeof(EmulatorInfo));
        memset(info, 0, sizeof(*info));
        emulate(bin, info);
    }

    if (do_rom) {
        string rom_data;
        string bin_data = { bin.len, bin.ptr };
        
        bool yes = modify_rom_subassembly(bin_data, &rom_data);
        if(!yes)
            return 1; // error already printed
        
        if(IS_SUBASSEMBLY(rom_file)) {
            if(DISABLE_FILE_WRITES) {
                printf("fopen '%s' write %d bytes\n", rom_file.ptr, rom_data.len);
            } else {
                FILE* file = fopen(rom_file.ptr, "wb");
                if(!file) {
                    error("Could not open '%s'.\n", rom_file.ptr);
                    return 1;
                }
                fwrite(rom_data.ptr, 1, rom_data.len, file);
                fclose(file);
            }
        } else {
            int slash_index = find_string(rom_file, "/");
            const char* title = nullptr;
            if(slash_index == -1) {
                // TODO: Memory leak
                char* tmp = malloc(500);
                sprintf(tmp, "C:/Program Files (x86)/Steam/steamapps/common/Logic World/subassemblies/%s", rom_file.ptr);
                rom_file.ptr = tmp;
                rom_file.len = strlen(tmp);
                title = rom_file.ptr + 75-2-1;
            } else {
                title = rom_file.ptr + slash_index + 1;
            }

            bool yes = write_metadata(rom_file, title, overwrite_user_subassembly);
            if(!yes)
                return 1; // error already printed

            // TODO: Memory leak
            char* partialworld_path = malloc(500);
            sprintf(partialworld_path, "%s/data.partialworld", rom_file.ptr);
            
            if(DISABLE_FILE_WRITES) {
                printf("fopen '%s' write %d bytes\n", partialworld_path, rom_data.len);
            } else {
                FILE* file = fopen(partialworld_path, "wb");
                if(!file) {
                    error("Could not open '%s'.\n", partialworld_path);
                    return 1;
                }
                fwrite(rom_data.ptr, 1, rom_data.len, file);
                fclose(file);
            }
        }
    }
    
    if (bin_file.ptr) {
        if(DISABLE_FILE_WRITES) {
            printf("fopen '%s' write %d bytes\n", bin_file.ptr, bin.len);
        } else {
            FILE* file = fopen(bin_file.ptr, "wb");
            if(!file) {
                error("Could not write to '%s'\n", bin_file.ptr);
                return 1;
            }
            int len = fwrite(bin.ptr, 1, bin.len, file);
            fclose(file);
        }
    }

    return 0;
}

// Declared in config.h
bool DISABLE_FILE_WRITES = false;
// bool DISABLE_FILE_WRITES = true;