
DEVICE_ROOT := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))).)
ROOT        := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/../../..)

IS_NIXOS := $(shell test -e /etc/nixos && echo yes)

ifeq ($(IS_NIXOS),yes)
# Raylib needed for display device (not emulator itself).
# Devices such as display will be compiled separately into shared libraries
# in the future meaning emulator won't need raylib dependency.
# On NixOS we use raylib specified by shell.nix. We also implicitly get include header.
# This won't work on Ubuntu...
LDFLAGS += -lraylib
else
# @TODO Paths on ubuntu? provide raylib binaries in repo for ubuntu-like installations.
CFLAGS += -I$(ROOT)/libs/raylib-6.0_linux_amd64/include
LDFLAGS += $(ROOT)/libs/raylib-6.0_linux_amd64/lib/libraylib.a
endif

include $(DEVICE_ROOT)/../template/Makefile
