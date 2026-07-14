
Platform configuration when running in emulator.


```toml
core_count   = 1
ram_size     = 1MB
cpu_family   = lwm16 # lwm32, lwm64
cpu_features = float
core_entry   = 0x0
devices      = [ "uart.so", "display.so" ]

rom_load_address = 0x0

# @TODO Shared libraries that emulate devices? Emulator sends MMIO, tick, trap events to device library?
```