# cliffs

Native SDL2 port of the 1991-1992 climbing game prototype "Attack the Cliffs."

## Build

```sh
make
```

## Run

```sh
./cliffs
./cliffs joe3
./cliffs levels/washing.ton
```

The loader accepts existing ledge files with or without extensions. It searches the path you pass first, then the `levels/` directory, and also tries `.ldg` when needed.

## Smoke Test

For a headless startup check:

```sh
SDL_VIDEODRIVER=dummy ./cliffs --smoke-test
```

## Controls

- `Enter`: start a round
- `Esc`: quit, or retreat from an active climb
- `F1`: select PeterB
- `F2`: select DougalH
- Arrow keys: select movement direction
- Left hand: `q` / `a`
- Left foot: `s` / `x`
- Right foot: `k` / `m`
- Right hand: `p` / `l`
- Belay rope to climber: `w`, `b`, `o`
- Belay rope to ledge: `e`, `i`
