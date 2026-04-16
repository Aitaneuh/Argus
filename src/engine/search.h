#pragma once
#include "chess.hpp"

int minimax(chess::Board &board, int depth, bool white_to_play, int alpha, int beta);
chess::Move search(chess::Board &board, int depth);