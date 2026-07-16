

#pragma once

#include "lwm/emulator.h"

void emulator_start(PlatformConfig* config);

bool parse_platform_config(const char* path, PlatformConfig* config);

void dump(PlatformConfig* config);
