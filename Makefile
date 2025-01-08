# This is the name of your final .nex file without the .nex extension
EXEC_OUTPUT=build/test

# List all your source files here
SOURCES = main.c GameObject.c gfx.c netcom.c uart2.c TextTileMap.c

# Maybe you'll need to edit this
CRT=1
#ZXNEXT_LAYER2_INCLUDE := $(ZXNEXT_LAYER2)/include
#ZXNEXT_LAYER2_LIB_SDCC_IY := $(ZXNEXT_LAYER2)/lib/sdcc_iy
#ZXNEXT_LAYER2_LIB_SDCC_IY := $(ZXNEXT_LAYER2)/lib/zxn

# You don't need to edit below here, have a nice day.

MKDIR = mkdir -p
CC=zcc
AS=zcc
TARGET=+zxn -subtype=nex
#VERBOSITY=-vn
VERBOSITY=-v
OUT_DIR=build bin
PRAGMA_FILE=zpragma.inc
#DEBUGFLAGS := --list --c-code-in-asm

OBJECTS=$(SOURCES:.*=.o)
OBJS=$(patsubst %, src/%, $(OBJECTS))

#C_OPT_FLAGS=-SO3 --max-allocs-per-node200000 --opt-code-size
C_OPT_FLAGS=-SO3 --max-allocs-per-node200000

CFLAGS=$(TARGET) $(VERBOSITY) -c $(C_OPT_FLAGS) -compiler sdcc -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE)
LDFLAGS=$(TARGET) $(VERBOSITY) --list -m -s -clib=sdcc_iy -llib/zxn/zxnext_layer2 -llib/zxn/zxnext_sprite -pragma-include:$(PRAGMA_FILE)
ASFLAGS=$(TARGET) $(VERBOSITY) -c --list -m -s

EXEC=$(EXEC_OUTPUT).nex

%.o: %.c $(PRAGMA_FILE)
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

all : dirs $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(LDFLAGS) -startup=$(CRT) $(OBJS) -o $(EXEC_OUTPUT) -create-app

.PHONY: clean dirs install

install: all
	# mv $(EXEC) bin

clean:
	rm -rf $(OUT_DIR) /tmp/tmpXX*
	rm -f src/*.lis src/*.sym src/*.o

dirs: $(OUT_DIR)

$(OUT_DIR):
	$(MKDIR) $(OUT_DIR)