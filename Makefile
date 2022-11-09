CC := gcc

CFLAGS := $(shell pkg-config --cflags gtk+-3.0 gtksourceview-3.0 libcjson)
LIBS := $(shell pkg-config --libs gtk+-3.0 gtksourceview-3.0 libcjson)

all: build/notepad

build/notepad: notepad.c
	mkdir -p build/
	${CC} ${CFLAGS} notepad.c -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
