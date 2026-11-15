#pragma once
#include "chess.hpp"
#include <optional>
#include <string>

// Score used for a tablebase win (below mate scores, above any normal eval).
constexpr int TB_WIN_SCORE = 50000;

// Load Syzygy tablebases from a folder (several folders can be separated by ';').
// Returns true if at least one table was found.
bool syzygy_init(const std::string &path);

// Largest piece count that the loaded tables cover (0 = no tables).
int syzygy_max_pieces();

// Root probe (uses DTZ tables): returns the best move and its score if the
// position is in the tablebase.
struct TBRootResult
{
    chess::Move move;
    int score;
};
std::optional<TBRootResult> syzygy_probe_root(const chess::Board &board);

// Search probe (uses WDL tables): returns a score from the side-to-move's
// point of view, or nothing if the position cannot be probed.
std::optional<int> syzygy_probe_wdl(const chess::Board &board);
