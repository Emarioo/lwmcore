
#include "lwmcore/assembler.h"
#include "lwmcore/emulator.h"

int main(int argc, const char** argv) {
    string file = {0};
    string out = {5, "prog.bin"};
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
            out.len = strlen(arg);
            out.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        } else {
            file.len = strlen(arg);
            file.ptr = (char*)arg; // TODO: Don't cast like this. Temporarily discarding the warning.
        }
    }

    bool file_is_asm = file.len>4 && !strcmp(file.ptr + file.len-4, ".asm");
    bool file_is_bin = file.len>4 && !strcmp(file.ptr + file.len-4, ".bin");

    if (!file_is_asm && !file_is_bin) {
        error("File must be .asm to assemble or .bin. Not '%s'.\n", file.ptr);
        return 1;
    }

    Bin bin;
    if (file_is_asm) {
        AssemblerInfo info;
        assemble(file, &bin, &info);
    }

    if (file_is_bin || (file_is_asm && do_emulate)) {
        EmulatorInfo info;
        emulate(bin, &info);
    }

    if (bin.len) {
        FILE* file = fopen(out.ptr, "w");
        if(!file) {
            error("Could not write to '%s'\n", out.ptr);
            return 1;
        }
        int len = fwrite(bin.ptr, 1, bin.len, file);
        fclose(file);
    }

    return 0;
}
