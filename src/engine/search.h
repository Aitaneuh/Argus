#pragma once
#include "chess.hpp"

struct SearchResult
{
    chess::Move best_move;
    int score_cp;
    int nodes;
    int nps;
};

int minimax(chess::Board &board, int depth, bool white_to_play, int alpha, int beta);
SearchResult search(chess::Board &board, int depth);