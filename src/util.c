#include "lwmcore/util.h"

bool equal(string text, const char* str) {
    return !strcmp(text.ptr, str);
}