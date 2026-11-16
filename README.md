# Argus

A UCI-compatible chess engine written in C++17, built from scratch with a focus on correctness and progressive optimization.

---

## Overview

Argus is a classical chess engine that uses an alpha-beta search with iterative deepening to find the best move in a given position. It communicates via the UCI protocol, meaning it plugs directly into any major chess GUI (CuteChess, Arena, etc.) without any extra configuration.

The engine is in active development. Latest STS estimate: **~2320 Elo (v4)**, see [Strength Estimate](#strength-estimate-sts).

---

## Features

- **UCI protocol** — plug into any GUI out of the box, search runs on its own thread (`stop` works)
- **Search** — negamax alpha-beta, iterative deepening, principal variation search,
  quiescence search, null move pruning, late move reductions, check extension
- **Time management** — uses `wtime/btime/winc/binc/movestogo/movetime`, or `go depth N` / `go infinite`
- **Transposition table** — configurable with the `Hash` option, mate-distance aware
- **Move ordering** — TT move, MVV-LVA captures, killer moves, history heuristic
- **Evaluation** — tapered piece-square tables (PeSTO values), middlegame/endgame blend
- **Draw detection** — repetition, 50-move rule, insufficient material
- **Opening book** — Polyglot `.bin` (`gm2001.bin`), varied move choice
- **Endgame tablebases** — Syzygy via Fathom (put the files in `build/syzygy/`)
- **Legal move generation** — via [chess-library by Disservin](https://github.com/Disservin/chess-library)

---

## Project Structure

```
Argus/
├── lib/
│   ├── chess-library/        # Disservin's chess-library (move generation)
│   ├── fathom/               # Syzygy tablebase probing
│   └── opening-book/         # gm2001.bin Polyglot book
├── src/
│   ├── main.cpp
│   ├── engine/
│   │   ├── search.h / search.cpp     # Iterative deepening, alpha-beta, qsearch, time
│   │   ├── evaluate.h / evaluate.cpp # Tapered PST evaluation
│   │   ├── movesort.h / movesort.cpp # Move ordering (TT move, MVV-LVA, killers, history)
│   │   ├── tt.h / tt.cpp             # Transposition table
│   │   ├── polyglot.*                # Opening book
│   │   └── syzygy.h / syzygy.cpp     # Endgame tablebases (Fathom wrapper)
│   └── uci/
│       └── uci.h / uci.cpp           # UCI protocol handler
├── tests/
│   ├── perft.cpp                     # Move generation correctness tests
│   └── sts/                          # STS test suite + Elo estimate script
└── CMakeLists.txt
```

---

## Building

### Requirements

- GCC 15+ or Clang with C++17 support
- CMake 3.16+
- Ninja (recommended)

> On Windows, use the **MSYS2 UCRT64** terminal. Install dependencies with:
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

| Depth | Nodes |
|-------|-------|
| 1 | 20 |
| 2 | 400 |
| 3 | 8,902 |
| 4 | 197,281 |
| 5 | 4,865,609 |
| 6 | 119,060,324 |

---

## Using with a GUI

1. Download [CuteChess](https://github.com/cutechess/cutechess/releases)
2. Go to **Tools → Engines → Add**
3. Point the executable path to `build/Argus.exe`
4. Set the working directory to `build/` and the protocol to **UCI**
5. Start a game (any time control works, Argus manages its clock)

UCI options: `Hash` (MB), `OwnBook`, `BookFile`, `SyzygyPath`.
By default Argus loads `gm2001.bin` and the `syzygy/` folder from next to the executable.

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
go movetime 1000
→ info depth 1 score cp 63 nodes 32 nps 0 time 0 pv g1f3
→ ...
→ info depth 13 score cp 25 nodes 6900000 nps 7500000 time 920 pv g1f3 b8c6 ...
→ bestmove g1f3
```

Plain `go` searches to depth 8, `go infinite` searches until `stop`.

---

## Strength Estimate (STS)

`tests/sts/sts.py` runs the [Strategic Test Suite](https://github.com/fsmosca/STS-Rating)
(1500 positions, 15 themes) and turns the score into a rough Elo estimate
(`Elo ≈ 44.523 × score% − 242.85`, about ±150). Build the engine first, then:

```bash
python tests/sts/sts.py          # full suite (~6 min, 0.2 s per position)
python tests/sts/sts.py -n 300   # quick run, 20 positions per theme
```

| Version | STS score | Estimated Elo |
|---------|-----------|---------------|
| v3 (material only, fixed depth 6) | 31.1% | ~1140 |
| v4 (PST, qsearch, iterative deepening, time management) | 57.6% | ~2320 |

---

## Roadmap

- [x] UCI communication
- [x] Legal move generation
- [x] Alpha-beta pruning
- [x] Material evaluation
- [X] MVV-LVA move ordering
- [x] Transposition table
- [x] Piece-square tables (PST, tapered)
- [x] Quiescence search
- [x] Iterative deepening
- [x] Time management
- [x] Null move pruning, LMR, PVS, killers, history
- [x] Opening book (Polyglot `.bin`)
- [x] Syzygy endgame tablebases (via Fathom)
- [ ] Pawn structure / king safety / mobility eval
- [ ] Aspiration windows, SEE, futility pruning

---

## Dependencies

- [chess-library](https://github.com/Disservin/chess-library) by Disservin — header-only move generation library
- [Fathom](https://github.com/jdart1/Fathom) — Syzygy tablebase probing
- [STS-Rating](https://github.com/fsmosca/STS-Rating) by fsmosca — STS positions and Elo formula

---

## License

MIT
