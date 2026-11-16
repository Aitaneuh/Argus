"""
Rough Elo estimate for Argus using the Strategic Test Suite (STS).

STS has 1500 positions in 15 themes (100 each). Every position gives
up to 10 points depending on which move the engine plays. The Elo estimate
uses the linear fit from fsmosca's STS-Rating tool:

    Elo ~= 44.523 * score% - 242.85   (calibrated on CCRL at ~0.2 s/move)

It is only a rough ballpark (about +/- 150 Elo), good enough to tell
a 1000 engine from a 2000 one.

Usage (from the project root):
    python tests/sts/sts.py                 # all 1500 positions
    python tests/sts/sts.py -n 300          # quick run: 20 positions per theme
    python tests/sts/sts.py -e path/to/Argus.exe
"""

import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DEFAULT_ENGINE = os.path.join(ROOT, "build", "Argus.exe" if os.name == "nt" else "Argus")
DEFAULT_EPD = os.path.join(HERE, "sts.epd")

SLOPE, INTERCEPT = 44.523, -242.85


def parse_epd(line):
    """Returns (fen, theme, {uci_move: points}) for one STS line."""
    parts = line.strip().split()
    fen = " ".join(parts[:4]) + " 0 1"  # EPD has no move counters
    ops = {}
    for op in " ".join(parts[4:]).split(";"):
        op = op.strip()
        if " " in op:
            key, val = op.split(" ", 1)
            ops[key] = val.strip('"')
    moves = ops["c9"].split()
    points = [int(p) for p in ops["c8"].split()]
    # id looks like 'STS(v3.0) Knight Outposts.042' -> theme "STS3 Knight Outposts"
    tag, name = ops["id"].split(")", 1)
    number = int(float(tag.split("v")[1]))
    theme = f"STS{number} {name.rsplit('.', 1)[0].strip()}" if number else name
    return fen, number, theme, dict(zip(moves, points))


class Engine:
    def __init__(self, path):
        self.p = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1, cwd=os.path.dirname(path))
        self.send("uci")
        self.wait_for("uciok")
        self.send("setoption name OwnBook value false")  # test the search, not the book
        self.send("isready")
        self.wait_for("readyok")

    def send(self, cmd):
        self.p.stdin.write(cmd + "\n")
        self.p.stdin.flush()

    def wait_for(self, prefix):
        while True:
            line = self.p.stdout.readline()
            if not line:
                sys.exit("engine exited unexpectedly")
            if line.startswith(prefix):
                return line.strip()

    def bestmove(self, fen):
        self.send("ucinewgame")
        self.send("position fen " + fen)
        self.send("go movetime 200")
        return self.wait_for("bestmove").split()[1]

    def quit(self):
        self.send("quit")
        self.p.wait(timeout=5)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-e", "--engine", default=DEFAULT_ENGINE)
    ap.add_argument("-f", "--epd", default=DEFAULT_EPD)
    ap.add_argument("-n", "--positions", type=int, default=1500,
                    help="number of positions, spread evenly over the 15 themes")
    args = ap.parse_args()

    if not os.path.exists(args.engine):
        sys.exit(f"engine not found: {args.engine} (build it first)")

    with open(args.epd) as f:
        positions = [parse_epd(l) for l in f if l.strip()]

    # Take the first k positions of every theme so short runs stay balanced.
    per_theme = max(1, args.positions // 15)
    by_theme, names = {}, {}
    for pos in positions:
        by_theme.setdefault(pos[1], []).append(pos)
        names.setdefault(pos[1], pos[2])
    selected = [p for theme in by_theme.values() for p in theme[:per_theme]]

    engine = Engine(os.path.abspath(args.engine))
    score, theme_score, theme_count = 0, {}, {}
    start = time.time()

    for i, (fen, theme, _, points) in enumerate(selected, 1):
        move = engine.bestmove(fen)
        pts = points.get(move, 0)
        score += pts
        theme_score[theme] = theme_score.get(theme, 0) + pts
        theme_count[theme] = theme_count.get(theme, 0) + 1
        pct = 100 * score / (10 * i)
        print(f"\r[{i}/{len(selected)}] score {score}/{10 * i} ({pct:.1f}%)  "
              f"est. Elo {SLOPE * pct + INTERCEPT:.0f}  ", end="", flush=True)

    engine.quit()
    elapsed = time.time() - start
    pct = 100 * score / (10 * len(selected))

    print("\n")
    print(f"{'Theme':50s} {'Score':>10s} {'%':>6s}")
    for theme in by_theme:
        if theme in theme_count:
            s, n = theme_score[theme], theme_count[theme]
            print(f"{names[theme]:50s} {s:>5d}/{10 * n:<4d} {100 * s / (10 * n):6.1f}")
    print()
    print(f"Positions : {len(selected)}  ({elapsed / len(selected):.2f} s per position)")
    print(f"Score     : {score}/{10 * len(selected)}  ({pct:.1f}%)")
    print(f"STS Elo   : ~{SLOPE * pct + INTERCEPT:.0f}  (rough estimate, +/- 150)")


if __name__ == "__main__":
    main()
