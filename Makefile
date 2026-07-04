#
# Makefile to build EFI Application
#

# Parameters to set:

ifeq ($(OS),Windows_NT)
  INT_DIR  ?= int/lwm_windows
  EXECUTABLE ?= $(INT_DIR)/lwm.exe
else
  INT_DIR  ?= int/lwm_linux
  EXECUTABLE ?= $(INT_DIR)/lwm
endif

SILENT ?= @

################################


ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CC := gcc

CFLAGS := -g 
CFLAGS += -I$(ROOT)/include -I$(ROOT)/src
CFLAGS += -Wall -Werror -fshort-wchar -Werror=implicit-function-declaration
CFLAGS += -Wno-multichar
CFLAGS += -Wno-unused-variable -Wno-unused-value -Wno-unused-function -Wno-unused-but-set-variable

LDFLAGS += -g

SRC_DIRS := \
	$(ROOT)/src

SRC_FILES := \
	$(foreach dir,$(SRC_DIRS),$(shell find $(dir) \( -name '*.c' -o -name '*.s' \) ))

OBJ_FILES := $(patsubst $(ROOT)/%.c,$(INT_DIR)/%.o,$(SRC_FILES))
OBJ_FILES := $(patsubst $(ROOT)/%.s,$(INT_DIR)/%.o,$(OBJ_FILES))
DEP_FILES := $(patsubst %.o,%.d,$(OBJ_FILES))

# $(info $(OBJ_FILES) $(SRC_FILES))

ifeq ($(OS),Windows_NT)
	RM_CMD := cmd /C del /Q
	MKDIR  := cmd /C mkdir
else
	RM_CMD := rm
	MKDIR  := mkdir -p
endif

all: $(EXECUTABLE)

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
	$(SILENT) $(CC) -o $@ $^ $(LDFLAGS)

clean:
ifeq ($(OS),Windows_NT)
	$(SILENT) $(RM_CMD) $(subst /,\,$(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE))
else
	$(SILENT) $(RM_CMD) $(OBJ_FILES) $(DEP_FILES) $(EXECUTABLE)
endif
