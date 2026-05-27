CC ?= cc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic $(shell $(PKG_CONFIG) --cflags sdl2)
LDLIBS += $(shell $(PKG_CONFIG) --libs sdl2) -lm

SRC := src/main.c src/game.c src/sdl_app.c
BIN := cliffs

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDLIBS)

clean:
	rm -f $(BIN)

.PHONY: all clean
