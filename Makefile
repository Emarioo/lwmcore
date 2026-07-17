#
# Makefile to build assembler/emulator
#

# Parameters to set:

APP_NAME := lwm

ifeq ($(OS),Windows_NT)
  BIN_DIR    ?= bin
  INT_DIR    ?= int/$(APP_NAME)_windows
  EXECUTABLE ?= $(INT_DIR)/$(APP_NAME).exe
else
  BIN_DIR    ?= bin
  INT_DIR    ?= int/$(APP_NAME)_linux
  EXECUTABLE ?= $(INT_DIR)/$(APP_NAME)
endif

SILENT ?= @
OFLAG  ?=
export OFLAG # export to device makefiles

################################


ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CC := gcc

CFLAGS := -g $(OFLAG)
CFLAGS += -I$(ROOT)/include -I$(ROOT)/src
CFLAGS += -Wall -Werror -fshort-wchar -Werror=implicit-function-declaration
CFLAGS += -Wno-multichar
CFLAGS += -Wno-unused-variable -Wno-unused-value -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-result

LDFLAGS += -g -lm 

# Raylib needed for display device (not emulator itself).
# Devices such as display will be compiled separately into shared libraries
# in the future meaning emulator won't need raylib dependency.
# On NixOS we use raylib specified by shell.nix. We also implicitly get include header.
# This won't work on Ubuntu...
LDFLAGS += -lraylib


SRC_DIRS := \
	$(ROOT)/src/lwm \
	$(ROOT)/src/common

SRC_FILES := \
	$(foreach dir,$(SRC_DIRS),$(shell find $(dir) \( -name '*.c' -o -name '*.s' \) ))

OBJ_FILES := $(patsubst $(ROOT)/%.c,$(INT_DIR)/%.o,$(SRC_FILES))
OBJ_FILES := $(patsubst $(ROOT)/%.s,$(INT_DIR)/%.o,$(OBJ_FILES))
DEP_FILES := $(patsubst %.o,%.d,$(OBJ_FILES))

# $(info $(OBJ_FILES) $(SRC_FILES))

ifeq ($(OS),Windows_NT)
	RM_CMD := cmd /C del /Q
	MKDIR  := cmd /C mkdir
	CP     := cp
else
	RM_CMD := rm -f
	MKDIR  := mkdir -p
	CP     := cp
endif


all: $(EXECUTABLE) devices

ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean)))
    -include $(DEP_FILES)
endif


$(INT_DIR):
ifeq ($(OS),Windows_NT)
	$(SILENT) $(MKDIR) $(subst /,\,$(INT_DIR))
else
	$(SILENT) $(MKDIR) $(INT_DIR)
endif

$(INT_DIR)/%.o: $(ROOT)/%.c | $(INT_DIR)
	$(SILENT) mkdir -p $(@D)
	$(SILENT) $(CC) $(CFLAGS) -c -MD -o $@ $<

$(INT_DIR)/%.o: $(INT_DIR)/%.s | $(INT_DIR)
	$(SILENT) mkdir -p $(@D)
	$(SILENT) $(AS) $(ASFLAGS) -c -MD $(patsubst %.o,%.d,$@) -o $@ $<

$(EXECUTABLE): $(OBJ_FILES) | $(INT_DIR)
	$(SILENT) mkdir -p $(BIN_DIR)
	$(SILENT) $(CC) -o $@ $^ $(LDFLAGS)
	$(SILENT) $(CP) $@ $(BIN_DIR)/$(notdir $@)


DEVICE_DIRS := \
	$(ROOT)/src/devices/display/Makefile	\
	$(ROOT)/src/devices/emu/Makefile		\
	$(ROOT)/src/devices/ic/Makefile		    \
	$(ROOT)/src/devices/platform/Makefile   \
	$(ROOT)/src/devices/uart/Makefile		\
	#

.PHONY: devices clean $(DEVICE_DIRS)


clean:
ifeq ($(OS),Windows_NT)
	$(SILENT) $(RM_CMD) $(subst /,\,$(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE))
else
	$(SILENT) $(RM_CMD) $(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE)
endif
	$(foreach dir,$(DEVICE_DIRS),$(MAKE) -f $(dir) clean;)


devices: $(DEVICE_DIRS)

$(DEVICE_DIRS):
	-$(MAKE) -f $@
