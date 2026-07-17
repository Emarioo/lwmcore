
#include "lwm/emulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <raylib.h>
#include <rlgl.h>

#include "lwm/util.h"


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


bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data);
bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data);

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

volatile bool hasRendered = false;
volatile bool doShutdown = false;

bool device_event(HardwareDevice* device, HardwareEvent eventType, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3) {
    EmulatorContext* emulator = device->emulator;
    Display_State*   state    = device->state;

    switch (eventType) {
        case EVENT_INIT: {
            state = malloc(sizeof(*state));
            memset(state, 0, sizeof(*state));

            device->state     = state;
            device->eventMask = 0;
            device->mmio_read = device_read;
            device->mmio_write = device_write;

            state->ram.width  = 640;
            state->ram.height = 480;
            state->ram.stride = state->ram.width * 4;
            state->frameBufferSize = state->ram.height * state->ram.stride;
            state->frameBuffer = malloc(state->frameBufferSize);
            memset(state->frameBuffer, 0xFF, state->frameBufferSize);

            declare_mmio(device, DISPLAY_BASE, 0x40);
            declare_mmio(device, DISPLAY_FRAMEBUFFER, state->frameBufferSize);

            hasRendered = false;
            doShutdown = false;

            int res = pthread_create(&state->thread, NULL, display_thread, state);
            if (res != 0) {
                printf("\033[31mERROR:\033[0m Could not create display thread\n");
            }

            // Give display thread some time to render stuff.
            // Otherwise emulator may render something and shutdown
            // before display thread has done anything.
            while (!hasRendered) {
                sleep_us(1000);
            }

            return true;
        } break;
        case EVENT_DEINIT: {
            doShutdown = true;
            pthread_join(state->thread, NULL);
            return true;
        } break;
        default:
    }
    return false;
}

bool device_write(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
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


bool device_read(HardwareDevice* device, uintptr_t address, size_t size, void* data) {
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
    
    while (!WindowShouldClose() && !doShutdown) {
        BeginDrawing();

        rlDisableColorBlend();
        
        UpdateTexture(state->texture, state->frameBuffer);
        // @TODO Upscaled frame buffer.
        DrawTexture(state->texture, 0, 0, WHITE);
        
        EndDrawing();
        hasRendered = true;
    }
    UnloadTexture(state->texture);

    CloseWindow();
    
    return NULL;
}