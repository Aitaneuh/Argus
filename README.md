# Argus

A UCI-compatible chess engine written in C++17, built from scratch with a focus on correctness and progressive optimization.

---

## Overview

Argus is a classical chess engine that uses alpha-beta pruning to search for the best move in a given position. It communicates via the UCI protocol, meaning it plugs directly into any major chess GUI (CuteChess, Arena, etc.) without any extra configuration.

The engine is in early development — the core search and eval are functional, and the architecture is designed to make adding stronger features straightforward.

---

## Features

- **UCI protocol** — fully implemented, plug into any GUI out of the box
- **Alpha-beta search** — negamax with alpha-beta pruning
- **Material evaluation** — centipawn-accurate piece values
- **Move ordering** — skeleton in place, MVV-LVA coming next
- **Legal move generation** — via [chess-library by Disservin](https://github.com/Disservin/chess-library)

---

## Project Structure

```
Argus/
├── lib/
│   └── chess-library/        # Disservin's chess-library (move generation)
├── src/
│   ├── main.cpp
│   ├── engine/
│   │   ├── search.h / search.cpp     # Alpha-beta search
│   │   ├── evaluate.h / evaluate.cpp # Position evaluation
│   │   └── movesort.h / movesort.cpp # Move ordering
│   └── uci/
│       └── uci.h / uci.cpp           # UCI protocol handler
├── tests/
│   └── perft.cpp                     # Move generation correctness tests
└── CMakeLists.txt
```

---

## Building

### Requirements

- GCC 15+ or Clang with C++17 support
- CMake 3.16+
- Ninja (recommended)

> On Windows, use the **MSYS2 UCRT64** terminal. Install dependencies with:
>
> ```bash
> pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
> ```

### Build

```bash
cmake -B build -G "Ninja"
cmake --build build
```

The engine binary will be at `build/Argus.exe` (Windows) or `build/Argus` (Linux/macOS).

### Run perft tests

```bash
cmake --build build --target Perft
./build/Perft
```

Expected output from the starting position:

| Depth | Nodes       |
| ----- | ----------- |
| 1     | 20          |
| 2     | 400         |
| 3     | 8,902       |
| 4     | 197,281     |
| 5     | 4,865,609   |
| 6     | 119,060,324 |

---

## Using with a GUI

1. Download [CuteChess](https://github.com/cutechess/cutechess/releases)
2. Go to **Tools → Engines → Add**
3. Point the executable path to `build/Argus.exe`
4. Set protocol to **UCI**
5. Start a game

---

## UCI Manual Mode

You can also talk to the engine directly in the terminal:

```
uci
→ id name Argus
→ id author Aitaneuh
→ uciok

isready
→ readyok

position startpos moves e2e4 e7e5
go
→ info depth 6 score cp 30 nodes 124000 nps 980000
→ bestmove g1f3
```

---

## Roadmap

- [x] UCI communication
- [x] Legal move generation
- [x] Alpha-beta pruning
- [x] Material evaluation
- [x] MVV-LVA move ordering
- [ ] Piece-square tables (PST)
- [ ] Iterative deepening
- [ ] Transposition table
- [ ] Time management
- [ ] Opening book

---

## Dependencies

- [chess-library](https://github.com/Disservin/chess-library) by Disservin — header-only move generation library

---

## License

MIT
