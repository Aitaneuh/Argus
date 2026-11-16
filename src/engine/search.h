#pragma once
#include "chess.hpp"
#include <atomic>
#include <cstdint>
#include <string>

// What the GUI asked for in "go ...". Times are in milliseconds, -1 = not given.
struct SearchLimits
{
    int depth = -1;          // go depth N
    int64_t movetime = -1;   // go movetime N
    int64_t time = -1;       // our remaining clock (wtime or btime)
    int64_t inc = 0;         // our increment (winc or binc)
    int movestogo = 0;       // moves until next time control (0 = sudden death)
    bool infinite = false;   // go infinite: search until "stop"
};

struct SearchResult
{
    chess::Move best_move;
    int score;               // centipawns, from the side to move's point of view
    int depth;               // last fully completed depth
    const char *source;      // "book", "syzygy" or "search"
};

// Mate scores: a mate in N plies from the root scores MATE_SCORE - N.
constexpr int MATE_SCORE = 100000;
constexpr int MATE_BOUND = MATE_SCORE - 1000; // |score| above this = forced mate

extern bool use_book;
extern std::atomic<bool> stop_search; // set by "stop"/"quit" to abort the search

// Iterative deepening search. Prints "info ..." lines while it searches.
SearchResult search(chess::Board board, const SearchLimits &limits);

// Thread-safe output to the GUI (defined in uci.cpp).
void uci_send(const std::string &line);
