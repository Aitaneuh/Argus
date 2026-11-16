#include "movesort.h"
#include "chess.hpp"
#include <algorithm>
#include <cstring>

using namespace chess;

// Scores must fit in Move::score(), which is an int16_t (max 32767).
static constexpr int SCORE_TT = 30000;
static constexpr int SCORE_CAPTURE = 20000;
static constexpr int SCORE_PROMO = 19000;
static constexpr int SCORE_KILLER1 = 18000;
static constexpr int SCORE_KILLER2 = 17000;
static constexpr int HISTORY_MAX = 16000;

static const int piece_values[] = {
    100, // PAWN
    320, // KNIGHT
    330, // BISHOP
    500, // ROOK
    900, // QUEEN
    0,   // KING
    0    // NONE
};

static uint16_t killers[MAX_PLY][2];
static int history[2][64][64]; // [color][from][to]

void clear_move_ordering()
{
    std::memset(killers, 0, sizeof(killers));
    std::memset(history, 0, sizeof(history));
}

static int score_move(const Board &board, const Move &move, uint16_t tt_move, int ply)
{
    if (move.move() == tt_move)
        return SCORE_TT;

    if (board.isCapture(move))
    {
        // En passant: the target square is empty but the victim is a pawn
        PieceType victim = move.typeOf() == Move::ENPASSANT ? PieceType(PieceType::PAWN)
                                                           : board.at(move.to()).type();
        PieceType attacker = board.at(move.from()).type();
        // victim counts much more than attacker: PxQ > QxQ > QxP
        return SCORE_CAPTURE + piece_values[victim] - piece_values[attacker] / 100;
    }

    if (move.typeOf() == Move::PROMOTION && move.promotionType() == PieceType::QUEEN)
        return SCORE_PROMO;

    if (ply < MAX_PLY)
    {
        if (killers[ply][0] == move.move()) return SCORE_KILLER1;
        if (killers[ply][1] == move.move()) return SCORE_KILLER2;
    }

    int c = static_cast<int>(board.sideToMove());
    return history[c][move.from().index()][move.to().index()];
}

void sort_moves(Movelist &moves, const Board &board, uint16_t tt_move, int ply)
{
    for (int i = 0; i < moves.size(); i++)
        moves[i].setScore(static_cast<int16_t>(score_move(board, moves[i], tt_move, ply)));

    std::stable_sort(moves.begin(), moves.end(), [](const Move &a, const Move &b)
                     { return a.score() > b.score(); });
}

void record_quiet_cutoff(const Board &board, const Move &move, int ply, int depth)
{
    if (ply < MAX_PLY && killers[ply][0] != move.move())
    {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = move.move();
    }

    int c = static_cast<int>(board.sideToMove());
    int &h = history[c][move.from().index()][move.to().index()];
    h += depth * depth;

    // Keep history below the killer scores: when one entry gets too big,
    // halve everything (old information slowly fades away).
    if (h > HISTORY_MAX)
        for (auto &side : history)
            for (auto &from : side)
                for (int &v : from)
                    v /= 2;
}
