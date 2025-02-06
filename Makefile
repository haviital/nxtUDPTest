# This is the name of your final .nex file without the .nex extension
EXEC_OUTPUT=build/NxtUdpTest

HDFMONKEY = c:\bin\hdfmonkey
EMUDIR = C:\bin\CSpect2_19_9_1
EMUSD = c:\bin\CSpect2_19_9_1\sn-emulator-22.10a.img
KILL = taskkill /F /IM
SYNCDIR = C:\nextsync

# List all your source files here
SOURCES = main.c GameObject.c gfx.c netcom.c uart2.c TextTileMap.c ui.c

# Maybe you'll need to edit this
CRT=1

# You don't need to edit below here, have a nice day.

MKDIR = mkdir -p
CC=zcc
AS=zcc
TARGET=+zxn -subtype=nex
VERBOSITY=-vn
#VERBOSITY=-v 
OUT_DIR=build bin
PRAGMA_FILE=zpragma.inc
#DEBUGFLAGS := --list --c-code-in-asm

OBJECTS=$(SOURCES:.*=.o)
OBJS=$(patsubst %, src/%, $(OBJECTS))

#C_OPT_FLAGS=-SO3 --max-allocs-per-node200000 --opt-code-size
C_OPT_FLAGS=-SO3 --max-allocs-per-node200000
## For debugging
#C_OPT_FLAGS=-SO3 --max-allocs-per-node200000 -On
CFLAGS=$(TARGET) $(VERBOSITY)  -c $(C_OPT_FLAGS) -compiler sdcc -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE)
LDFLAGS=$(TARGET) $(VERBOSITY) -DZXNEXT_EMULATOR_MODE_INCLUDES_$(ZXNEXT_EMULATOR_MODE_INCLUDES) --list -m -s -clib=sdcc_iy -llib/zxn/zxnext_layer2 -llib/zxn/zxnext_sprite -pragma-include:$(PRAGMA_FILE)
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

post-build:
	C:\bin\Z88CSPECT.EXE $(EXEC_OUTPUT).map $(EXEC_OUTPUT)_cspect.map
	# $(HDFMONKEY) put $(EMUSD) $(EXEC_OUTPUT).nex home
	# -$(KILL) cspect.exe 
	# CMD /C start /d $(EMUDIR) cspect -fullscreen -zxnext -nextrom -basickeys -exit -brk -vsync -tv -emu -mmc=$(EMUSD) -map=C:\git\nxtUDPTest\build\NxtUdpTest_cspect.map

sync:
	# -$(MD) "$(SYNCDIR)\NextLayer2Examples"
	cp "$(EXEC_OUTPUT).nex" "$(SYNCDIR)/home/"
	# -$(MD) "$(SYNCDIR)\NextZXOS"
	# copy "$(BASICDIR)\autoexec.bas" "$(SYNCDIR)\NextZXOS\*.*"             

	# Warning: kills all running python processes
	# Remove these two lines and start the server manually if that bothers you
	-$(KILL) python3.9.exe
	CMD /C start /d $(SYNCDIR) /min python3.9.exe nextsync.py

clean:
	rm -f $(EXEC_OUTPUT).*
	rm -f src/*.lis src/*.sym src/*.o

dirs: $(OUT_DIR)

$(OUT_DIR):
	$(MKDIR) $(OUT_DIR)