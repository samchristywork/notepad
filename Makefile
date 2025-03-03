CFLAGS = -Wall -Wextra -Wno-missing-field-initializers -std=c11 $(shell pkg-config --cflags gtk4)
LDFLAGS = $(shell pkg-config --libs gtk4)

.PHONY: all

all: build/notepad

build/notepad: $(patsubst src/%.c, build/%.o, $(wildcard src/*.c))
	gcc $^ $(LDFLAGS) -o $@

build/%.o: src/%.c | build
	gcc $(CFLAGS) -c $< -o $@

build:
	mkdir -p build
