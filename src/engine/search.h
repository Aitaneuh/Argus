#pragma once
#include "chess.hpp"

chess::Move search(chess::Board board, chess::Movelist legal_moves, int depth);
float minimax(Board board, int depth, bool white_to_play, float alpha, float beta);