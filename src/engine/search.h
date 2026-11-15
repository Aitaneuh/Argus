#pragma once
#include "chess.hpp"

struct SearchResult
{
    chess::Move best_move;
    int score_cp;
    int nodes;
    int nps;
    const char *source; // "book", "syzygy" or "search"
};

extern bool use_book;

int negamax(chess::Board &board, int depth, int alpha, int beta);
SearchResult search(chess::Board &board, int depth);