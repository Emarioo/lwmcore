#include "lwm/util.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include "Windows.h"
#else
    #include <sys/stat.h>
    #include <time.h>
    #include <unistd.h>
    #include <dlfcn.h>
#endif

bool equal(string text, const char* str) {
    return !strcmp(text.ptr, str);
}

bool endswith(string text, const char* str) {
    int len = strlen(str);
    return text.len > len && !strcmp(text.ptr + text.len - len, str);
}

int find_string(string text, const char* str) {
    int len = strlen(str);
    Assert(len > 0);
    int correct = 0;
    for(int i=0;i<text.len;i++) {
        char chr = text.ptr[i];
        if(chr == str[correct]) {
            correct++;
            if(correct == len)
                return i - len + 1;
        } else {
            correct = 0;
        }
    }
    return -1;
}

bool create_folder(string path) {
    #ifdef _WIN32
        // TODO: Make necessary parent directories
        BOOL res = CreateDirectoryA(path.ptr, NULL);
        if(!res) {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                error("Cannot create folder %s\n", path.ptr);
                return false;
            }
        }
        return true;
    #else
        // 7 = read, write, execute permissions for owner
        // 5 = read, execute for others
        int res = mkdir(path.ptr, 0755);
        if (res < 0) {
            error("Cannot create folder %s\n", path.ptr);
            return false;
        }
        return true;
    #endif
}

// bool remove_file(string path) {
//     // DUDE removing files in a directory should not be much code but it's like 50-60 lines.
//     #ifdef _WIN32
//         DWORD attributes = GetFileAttributesA(path.ptr);
//         if(attributes == INVALID_FILE_ATTRIBUTES){
//             DWORD err = GetLastError();
// 			if(err==ERROR_FILE_NOT_FOUND)
// 				return true;
//             error("Could not delete '%s'\n", path.ptr);
//             return false;
//         }
//         if (attributes & FILE_ATTRIBUTE_DIRECTORY) {

//         } else {
//             // TODO: Don't assume file?
//             bool yes = DeleteFileA(path.ptr);
//             if(!yes){
//                 DWORD err = GetLastError();
//                 if(err == ERROR_FILE_NOT_FOUND) {
//                     return true;
//                 }
//                 error("Cannot delete '%s'\n", path.ptr);
//                 return false;
//             }
//             return true;
//         }
//     #else

//     #endif
// }



bool readFile(const char* path, void** buffer, size_t* size) {

    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* fileBuffer = malloc(fileSize + 1);
    int readBytes = fread(fileBuffer, 1, fileSize, file);
    if (readBytes != fileSize) {
        fprintf(stderr, "Cannot read %zu bytes from %s\n", fileSize, path);
        return false;
    }
    fclose(file);
    fileBuffer[fileSize] = 0;

    *buffer = fileBuffer;
    *size = fileSize;
    return true;
}

bool writeFile(const char* path, void* buffer, size_t size) {

    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "Cannot open %s\n", path);
        return false;
    }
    int writtenBytes = fwrite(buffer, 1, size, file);
    if (writtenBytes != size) {
        fprintf(stderr, "Cannot write %zu bytes to %s\n", size, path);
        return false;
    }
    fclose(file);

    return true;
}


void sleep_us(uint64_t microseconds) {
    #ifdef _WIN32
        Sleep(microseconds / 1000);
    #else
        usleep(microseconds);
    #endif
}

uint64_t ticks_per_ms;

static uint64_t calibrate_timestamp() {
    struct timespec start_tp;
    struct timespec end_tp;
    uint64_t start;
    uint64_t end;
    
    start = timestamp();
    clock_gettime(CLOCK_MONOTONIC, &start_tp);
    
    sleep_us(10*1000); // 10ms
    
    end = timestamp();
    clock_gettime(CLOCK_MONOTONIC, &end_tp);

    uint64_t ns = (end_tp.tv_sec - start_tp.tv_sec) * 1000000000 + (end_tp.tv_nsec - start_tp.tv_nsec);
    uint64_t tpms = ((end - start) * 1000000) / ns;
    return tpms;
}

uint64_t timestamp_to_ns(uint64_t ts) {
    // @TODO If ts is large then we get precision issues when multiplying 1000000.
    //   If ts fits in 44 bits then we are okay. (2^20 ~= 1000000).
    if (!ticks_per_ms) {
        ticks_per_ms = calibrate_timestamp();
    }
    return (ts * 1000000) / ticks_per_ms;
}


DynamicLibrary load_library(const char* path) {
    #ifdef _WIN32
        HMODULE module = LoadLibraryA(path);
        return (DynamicLibrary)module;
    #else
        void* lib = dlopen(path, RTLD_LAZY);
        return lib;
    #endif
}
void unload_library(DynamicLibrary library) {
    #ifdef _WIN32
        FreeLibrary((HMODULE)library)
    #else
        dlclose(library);
    #endif
    
}
void* symbol_from_library(DynamicLibrary library, const char* name) {
    #ifdef _WIN32
        FARPROC proc = GetProcAddress((HMODULE)library, name);
        return (void*)proc;

    #else
        return dlsym(library, name);
    #endif
}
