
Platform configuration when running in emulator.


```toml
core_count   = 1
ram_size     = 1MB
cpu_family   = lwm16 # lwm32, lwm64
cpu_features = float
boot_entry   = 0x0

default_binary_load_address = 0x0

# @TODO Memory mapped file, log/uart like thing
# @TODO Shared libraries that emulate devices? Emulator sends MMIO, tick, trap events to device library?
```