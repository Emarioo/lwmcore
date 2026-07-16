
DEVICE_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))).)

# Raylib needed for display device (not emulator itself).
# Devices such as display will be compiled separately into shared libraries
# in the future meaning emulator won't need raylib dependency.
# On NixOS we use raylib specified by shell.nix. We also implicitly get include header.
# This won't work on Ubuntu...
LDFLAGS += -lraylib

include $(DEVICE_ROOT)/../template/Makefile
