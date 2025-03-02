OBJS_STANDALONE=main.o elf.o emulate.o emulate-instrs.o helper.o channel.o gdb.o
TARGET_STANDALONE=anergistic

OBJS_PYTHON=python.o emulate.o emulate-instrs.o helper.o channel.o gdb.o
TARGET_PYTHON=anergistic.so

UNAME=$(shell uname -s)
WINDOWSID=MINGW32_NT-6.1

ifeq ($(UNAME), $(WINDOWSID))
INCLUDE_PYTHON=C:\Python310\include
EXEC_GENERATE=python instr-generate.py
LIBS=-lws2_32
else
INCLUDE_PYTHON=/usr/include/python3.10/
EXEC_GENERATE=./instr-generate.py
LIBS=-lm
endif

DEPS=Makefile emulate-instrs.h config.h types.h

CC=clang
CFLAGS=-W -Wall -Wextra -fPIC -fno-strict-aliasing -O2 -I $(INCLUDE_PYTHON)
LDFLAGS=

ifeq ($(UNAME), $(WINDOWSID))
LIBRARY_PATH = C:\Python310\libs\
all: $(TARGET_STANDALONE) $(TARGET_PYTHON)
else
all: $(TARGET_STANDALONE) $(TARGET_PYTHON)
endif

$(TARGET_STANDALONE): $(OBJS_STANDALONE) $(DEPS)
	$(CC) -o $@ $(OBJS_STANDALONE) $(LIBS)

$(TARGET_PYTHON): $(OBJS_PYTHON) $(DEPS)
	$(CC) -o $@ $(OBJS_PYTHON) $(LIBS) -lpython3.10 -shared

%.o: %.c $(DEPS)
	$(CC) -c $(CFLAGS) -o $@ $<

emulate-instrs.h: emulate-instrs.h.in instrs instr-generate.py emulate-instrs.c.in
	$(EXEC_GENERATE) instrs emulate-instrs.h emulate-instrs.c

emulate-instrs.c: emulate-instrs.h.in instrs instr-generate.py emulate-instrs.c.in
	$(EXEC_GENERATE) instrs emulate-instrs.h emulate-instrs.c

clean:
	-rm -f $(TARGET_STANDALONE) $(TARGET_PYTHON) $(OBJS_STANDALONE) $(OBJS_PYTHON) emulate-instrs.h emulate-instrs.c
