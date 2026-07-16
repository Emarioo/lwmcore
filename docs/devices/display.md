
A display device that implements a framebuffer the program can draw pixels to.

The backend uses raylib. It can be switched out since the program doesn't depend on it.

The program must run in 32-bit mode or above. The MMIO addresses are placed above 16-bit address space.
You could support 16-bit but you would have a very small screen. 16-bit CPU should support 22-bit address space,
at least paging does but you can't fit 22-bit pointer in 16-bit register so it can be akward to work with.
Hence requirement for 32-bit.

# MMIO

```arm
#define DISPLAY_BASE          0xD0000
#define DISPLAY_WIDTH         (DISPLAY_BASE+0x0)
#define DISPLAY_HEIGHT        (DISPLAY_BASE+0x4)
#define DISPLAY_STRIDE        (DISPLAY_BASE+0x8)
// pixel format?
#define DISPLAY_FRAMEBUFFER   (DISPLAY_BASE+0x40)
```

