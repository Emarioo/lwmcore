
#include "lwm/assembler.h"
#include "lwm/emulator.h"
#include "lwm/blotter.h"
#include "lwm/scheme_gen.h"


// @TODO Move these elsewhere!
bool dev_create_emu(EmulatorContext* emulator, HardwareDevice* device);
bool dev_create_ic(EmulatorContext* emulator, HardwareDevice* device);
bool dev_create_uart(EmulatorContext* emulator, HardwareDevice* device);
bool platform_init(EmulatorContext* emulator, HardwareDevice* device);

void print_help() {
    printf("Usage: lwm sample.s -o sample.bin\n");
    printf("  -o <path>       : Path where to place output binary file.\n");
    printf("  -e,--emulate    : Assembles and emulates the file.\n");
    printf("  -r,--rom <path> : Assembles and writes a Logic World subassembly file.\n");
    printf("  -f,--force      : Will overwrite user made subassembly.\n");
    printf("  -s <path>       : Generate encoder from scheme.\n");
    printf("  --safe          : Don't write any files, log which ones would have been written to.\n");
    printf("  -q,--quiet      : Turn off log messages.\n");
}

int main(int argc, const char** argv) {
    string assembly_file = {0};
    string bin_file = {0, nullptr};
    string rom_file = {0, nullptr};
    const char* scheme_file = NULL;
    bool do_emulate = false;
    bool do_rom = false;
    bool do_scheme = false;
    bool overwrite_user_subassembly = false;
    bool be_quiet = false;
    bool verbose = false;

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
            }
            return 0;
        } else if(!strcmp(arg, "-e") || !strcmp(arg, "--emulate")) {
            do_emulate = true;
        } else if(!strcmp(arg, "-f") || !strcmp(arg, "--force")) {
            overwrite_user_subassembly = true;
        } else if(!strcmp(arg, "--safe")) {
            DISABLE_FILE_WRITES = true;
        } else if(!strcmp(arg, "-q") || !strcmp(arg, "--quiet")) {
            be_quiet = true;
        } else if(!strcmp(arg, "-d")) {
            verbose = true;
        } else if(!strcmp(arg, "-s")) {
            if(i+1 >= argc) {
                error("Missing argument after '%s'\n", arg);
                return 1;
            }
            do_scheme = true;
            arg = argv[i+1];
            i++;
            scheme_file = arg;
        } else if(!strcmp(arg, "-r") || !strcmp(arg, "--rom")) {
            if(i+1 >= argc) {
                error("Missing argument after '%s'\n", arg);
                return 1;
            }
            do_rom = true;
            arg = argv[i+1];
            i++;
            rom_file.len = strlen(arg);
            rom_file.ptr = (char*)arg; // TODO: Don't cast like
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

    if (do_scheme) {
        scheme_Database db;
        parse_scheme(scheme_file, &db);
        return 0;
    }

    if(assembly_file.len == 0) {
        print_help();
        return 0;
    }

    #define IS_SUBASSEMBLY(P) endswith(P, ".logicworld") || endswith(P, ".partialworld") || endswith(P, ".lwsubassembly")

    bool file_is_asm = endswith(assembly_file, ".asm") || endswith(assembly_file, ".s") || endswith(assembly_file, ".S");
    bool file_is_bin = endswith(assembly_file, ".bin");
    bool file_is_subassembly = IS_SUBASSEMBLY(assembly_file);

    if (!file_is_asm && !file_is_bin && !file_is_subassembly) {
        error("File must be .asm to assemble or .bin to emulate or .logicworld/.partialworld to deserialize. Not '%s'.\n", assembly_file.ptr);
        return 1;
    }

    PlatformConfig config = {0};
    config.quiet = be_quiet;
    config.verbose = verbose;

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
        buffer.ptr = (char*)malloc(buffer.len + 1);
        fread(buffer.ptr, 1, buffer.len, file);
        fclose(file);
        buffer.ptr[buffer.len] = 0;
        
        AssemblerOptions options = {0};
        options.quiet = be_quiet;
        options.verbose = verbose;
        options.sourcePath = assembly_file.ptr;
        if (bin_file.len) {
            options.outputPath = bin_file.ptr;
        } else {
            options.outputPath = "temp.bin";
        }
        AssemblerError result = assemble(buffer.ptr, buffer.len, &options);
        if (result != LWM_ASM_ERROR_NONE) {
            return 1;
        }

        config.rom = options.rom;
        config.rom_len = options.rom_len;

        if (!config.quiet) {
            dump(&config);
        }
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

        config.rom = buffer.ptr;
        config.rom_len = buffer.len;
    }

    if (do_emulate) {
        
        FN_init deviceConstructors[] = {
            dev_create_emu,
            dev_create_ic,
            dev_create_uart,
            platform_init,
        };
        
        config.devices_len = ARRAY_LENGTH(deviceConstructors);
        config.devices = malloc(sizeof(HardwareDevice*) * config.devices_len);
        memset(config.devices, 0, sizeof(HardwareDevice*) * config.devices_len);

        for (int i=0;i<config.devices_len;i++) {
            HardwareDevice* device = calloc(1, sizeof(HardwareDevice));
            config.devices[i] = device;
            device->init = deviceConstructors[i];
        }
        
        config.core_count = 2;
        config.core_mode = MODE_16;
        emulator_start(&config);
    }

    if (do_rom) {
        string rom_data;
        string bin_data = { config.rom_len, config.rom };
        
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
            printf("fopen '%s' write %d bytes\n", bin_file.ptr, config.rom_len);
        } else {
            FILE* file = fopen(bin_file.ptr, "wb");
            if(!file) {
                error("Could not write to '%s'\n", bin_file.ptr);
                return 1;
            }
            int len = fwrite(config.rom, 1, config.rom_len, file);
            fclose(file);
        }
    }

    return 0;
}

// Declared in config.h
bool DISABLE_FILE_WRITES = false;
// bool DISABLE_FILE_WRITES = true;
