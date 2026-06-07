# Bubble Bombing Bricks (Triple B)

I made this project during my first semester. The constraints included no libraries except iostream.

A terminal brick-breaker game using ANSI escape codes and raw terminal input.

## Build & run
```bash
g++ -o triple_b triple_b.cpp
./triple_b
```

## Controls
| Key | Action |
|-----|--------|
| `A` | Move cannon left |
| `D` | Move cannon right |
| `Q` | Aim left diagonal |
| `W` | Aim straight up (default) |
| `E` | Aim right diagonal |
| `SPACE` | Fire (costs 1 ammo) |
| `ESC` | Quit |

## Rules
- 50 ammo total. Clear all bricks to **win**; run out of ammo to **lose**.
- Each brick has 3 HP: `#` → `=` → `-` → destroyed. +10 points per hit.

## Allowed libraries
`iostream`, `unistd.h` (`usleep`, `read`, `STDIN_FILENO`),
`termios.h` (raw input setup), `cstdlib` (`system("clear")`)

## Architecture
| Function | Responsibility |
|----------|---------------|
| `initializeBricks` | Reset brick grid to full health |
| `renderBricks` | Write brick chars into screen buffer |
| `checkBrickHit` | Detect ball–brick collision and update state |
| `bricksRemaining` | Check win condition |
| `getBrickColor` | Select ANSI foreground color by HP |
| `renderScreen` | Flush screen buffer to terminal |
| `runGame` | Main game loop (input → physics → render) |
| `getch_linux` | Non-blocking raw character read |
