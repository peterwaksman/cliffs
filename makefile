CC := clang
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 $(shell pkg-config --cflags sdl2)
LDFLAGS := $(shell pkg-config --libs sdl2) -lm
SRC := src/main.c src/game.c src/sdl_app.c
BIN := cliffs

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BIN)

.PHONY: all clean
