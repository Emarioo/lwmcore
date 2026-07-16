
#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <raylib.h>
#include <rlgl.h>


//########################
//    Memory Mapped IO
//########################


#define DISPLAY_BASE          0xD0000
#define DISPLAY_WIDTH         (DISPLAY_BASE+0x0)
#define DISPLAY_HEIGHT        (DISPLAY_BASE+0x4)
#define DISPLAY_STRIDE        (DISPLAY_BASE+0x8)
// pixel format?
#define DISPLAY_FRAMEBUFFER   (DISPLAY_BASE+0x40)


//########################
//    Device Functions
//########################


bool device_init(EmulatorContext* emulator, HardwareDevice* device);
bool display_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool display_mmio_read(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data);
void display_tick(EmulatorContext* emulator, HardwareDevice* device);

// internal
void* display_thread(void* arg);


//########################
//    Implementation
//########################


typedef struct {
    uint32_t  width;
    uint32_t  height;
    uint32_t  stride;
} Display_RAM;


typedef struct {
    Display_RAM ram;
    pthread_t   thread;
    Texture2D   texture;
    uint8_t*    frameBuffer;
    uint32_t    frameBufferSize;
} Display_State;


bool device_init(EmulatorContext* emulator, HardwareDevice* device) {
    Display_State* state = malloc(sizeof(*state));
    memset(state, 0, sizeof(*state));

    device->state           = state;
    device->mmio_write      = display_mmio_write;
    device->mmio_read       = display_mmio_read;

    state->ram.width = 320;
    state->ram.height = 240;
    state->ram.stride = state->ram.width * 4;
    state->frameBufferSize = state->ram.height * state->ram.stride;
    state->frameBuffer = malloc(state->frameBufferSize);
    memset(state->frameBuffer, 0xFF, state->frameBufferSize);

    int res = pthread_create(&state->thread, NULL, display_thread, state);
    if (res != 0) {
        printf("\033[31mERROR:\033[0m Could not create display thread\n");
    }

    return true;
}


bool display_mmio_write(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    Display_State* state = device->state;
    // printf("PLATFORM MMIO 0x%zx %zd %c\n", address, size, *(char*)data);
    
    if (size < 1 || size > 8)
        return false;

    if (address >= DISPLAY_FRAMEBUFFER && address <= DISPLAY_FRAMEBUFFER + state->frameBufferSize - size) {
        uint8_t* dst = state->frameBuffer + address - DISPLAY_FRAMEBUFFER;
        memcpy(dst, data, size);
        return true;
    }

    return false;
}


bool display_mmio_read(EmulatorContext* emulator, HardwareDevice* device, uintptr_t address, size_t size, void* data) {
    Display_State* state = device->state;
    // printf("IC MMIO 0x%zx %zd %c\n", address, size, *(char*)data);

    if (size < 1 || size > 8)
        return false;

    if (address >= DISPLAY_BASE && address <= DISPLAY_BASE + sizeof(Display_RAM) - size) {
        memcpy(data, (char*)&state->ram + address - DISPLAY_BASE, size);
        return true;
    }
    return false;
}


//###########################
//    Rendering backend
//###########################

void* display_thread(void* arg) {
    Display_State* state = arg;

    SetTraceLogLevel(LOG_NONE);
    InitWindow(state->ram.width, state->ram.height, "LWM Display Device");

    SetTargetFPS(60);

    Image img = {
        .data = state->frameBuffer,
        .width = state->ram.width,
        .height = state->ram.height,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };

    state->texture = LoadTextureFromImage(img);
    
    while (!WindowShouldClose()) {
        BeginDrawing();

        rlDisableColorBlend();
        
        UpdateTexture(state->texture, state->frameBuffer);
        DrawTexture(state->texture, 0, 0, WHITE);
        
        EndDrawing();
    }

    printf("Display Thread Terminated by user.\n");

    return NULL;
}