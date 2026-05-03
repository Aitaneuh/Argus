#include "movesort.h"
#include "chess.hpp"

using namespace chess;

static const int piece_values[] = {
    100, // PAWN
    320, // KNIGHT
    330, // BISHOP
    500, // ROOK
    900, // QUEEN
    0,   // KING
    0    // NONE
};

static int score_move(const Board &board, const Move &move)
{
    PieceType attacker = board.at(move.from()).type();
    Piece victim_piece = board.at(move.to());

    if (victim_piece != Piece::NONE)
    {
        PieceType victim = victim_piece.type();
        int score = piece_values[victim] - piece_values[attacker];
        return board.sideToMove() == Color::WHITE ? score : -score;
    }

    return 0;
}

void sort_moves(Movelist &moves, Board &board)
{
    for (int i = 0; i < moves.size(); i++)
        moves[i].setScore(score_move(board, moves[i]));

    std::sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
              { return a.score() > b.score(); });
}