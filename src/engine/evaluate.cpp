#include "chess.hpp"
#include "evaluate.h"

using namespace chess;

int evaluate(Board &board)
{
    int score = 0;

    score += 100 * board.pieces(PieceType::PAWN, Color::WHITE).count();
    score -= 100 * board.pieces(PieceType::PAWN, Color::BLACK).count();
    score += 320 * board.pieces(PieceType::KNIGHT, Color::WHITE).count();
    score -= 320 * board.pieces(PieceType::KNIGHT, Color::BLACK).count();
    score += 330 * board.pieces(PieceType::BISHOP, Color::WHITE).count();
    score -= 330 * board.pieces(PieceType::BISHOP, Color::BLACK).count();
    score += 500 * board.pieces(PieceType::ROOK, Color::WHITE).count();
    score -= 500 * board.pieces(PieceType::ROOK, Color::BLACK).count();
    score += 900 * board.pieces(PieceType::QUEEN, Color::WHITE).count();
    score -= 900 * board.pieces(PieceType::QUEEN, Color::BLACK).count();

    return score;
}