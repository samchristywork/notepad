CC := g++

CFLAGS := $(shell pkg-config --cflags gtk+-3.0 gtksourceview-3.0 libcjson) -g
LIBS := $(shell pkg-config --libs gtk+-3.0 gtksourceview-3.0 libcjson) -lboost_filesystem

all: build/notepad

build/notepad: notepad.cpp
	mkdir -p build/
	${CC} ${CFLAGS} notepad.cpp -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
