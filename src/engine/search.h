#pragma once
#include "chess.hpp"

float minimax(chess::Board board, int depth, bool white_to_play, float alpha, float beta);
chess::Move search(chess::Board board, chess::Movelist legal_moves, int depth);