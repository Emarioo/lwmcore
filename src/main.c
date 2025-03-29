
#include "lwmcore/assembler.h"
#include "lwmcore/emulator.h"

int main(int argc, const char** argv) {
    string assembly_file = {0};
    string out_file = {5, "prog.bin"};
    bool do_emulate = false;

    for(int i=1;i<argc;i++) {
        const char* arg = argv[i];
        if(!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            printf("Usage: lwm sample.asm -o sample.bin\n");
            printf("  -o <path> : Path where to place output binary file. 'prog.bin' is default.\n");
            return 0;
        } else if(!strcmp(arg, "-e")) {
            do_emulate = true;
        } else if(!strcmp(arg, "-o")) {
            out_file.len = strlen(arg);
            out_file.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        } else {
            assembly_file.len = strlen(arg);
            assembly_file.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        }
    }

    bool file_is_asm = assembly_file.len>4 && !strcmp(assembly_file.ptr + assembly_file.len-4, ".asm");
    bool file_is_bin = assembly_file.len>4 && !strcmp(assembly_file.ptr + assembly_file.len-4, ".bin");

    if (!file_is_asm && !file_is_bin) {
        error("File must be .asm to assemble or .bin. Not '%s'.\n", assembly_file.ptr);
        return 1;
    }

    Bin bin;
    if (file_is_asm) {
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
    }

    if (file_is_bin || (file_is_asm && do_emulate)) {
        EmulatorInfo* info = (EmulatorInfo*)malloc(sizeof(EmulatorInfo));
        memset(info, 0, sizeof(*info));
        emulate(bin, info);
    }

    if (bin.len) {
        FILE* file = fopen(out_file.ptr, "w");
        if(!file) {
            error("Could not write to '%s'\n", out_file.ptr);
            return 1;
        }
        int len = fwrite(bin.ptr, 1, bin.len, file);
        fclose(file);
    }

    return 0;
}
