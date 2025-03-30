#include "lwmcore/util.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include "Windows.h"
#else
    #include <sys/stat.h>
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
        BOOL res = CreateDirectoryA(path.ptr, nullptr);
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
        int res = mkdir(path.c_str(), 0755);
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