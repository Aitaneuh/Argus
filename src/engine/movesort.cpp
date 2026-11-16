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
static constexpr int SCORE_BAD_CAPTURE = -2000; // + MVV-LVA, stays negative

static const int piece_values[] = {
    100, // PAWN
    320, // KNIGHT
    330, // BISHOP
    500, // ROOK
    900, // QUEEN
    0,   // KING
    0    // NONE
};

int see_value(PieceType pt) { return piece_values[static_cast<int>(pt)]; }

// All pieces (both colors) attacking `sq`, given the occupancy `occ`.
static Bitboard attackers_to(const Board &board, Square sq, Bitboard occ)
{
    Bitboard bishops = board.pieces(PieceType::BISHOP) | board.pieces(PieceType::QUEEN);
    Bitboard rooks = board.pieces(PieceType::ROOK) | board.pieces(PieceType::QUEEN);
    return ((attacks::pawn(Color::BLACK, sq) & board.pieces(PieceType::PAWN, Color::WHITE)) |
            (attacks::pawn(Color::WHITE, sq) & board.pieces(PieceType::PAWN, Color::BLACK)) |
            (attacks::knight(sq) & board.pieces(PieceType::KNIGHT)) |
            (attacks::bishop(sq, occ) & bishops) | (attacks::rook(sq, occ) & rooks) |
            (attacks::king(sq) & board.pieces(PieceType::KING))) &
           occ;
}

// Swap algorithm: both sides alternately recapture on the target square with
// their least valuable attacker. `swap` tracks how much the side to move is
// above the threshold; whoever runs out of profitable recaptures stops.
// Sliders behind the capturing piece (x-rays) join in as pieces leave.
// Pins are ignored, so it is an approximation, but a very good one.
bool see_ge(const Board &board, const Move &move, int threshold)
{
    // Castling / promotion / en passant: don't bother, treat as neutral.
    if (move.typeOf() != Move::NORMAL) return 0 >= threshold;

    const Square from = move.from(), to = move.to();

    int swap = see_value(board.at(to).type()) - threshold;
    if (swap < 0) return false; // even winning the piece for free isn't enough

    swap = see_value(board.at(from).type()) - swap;
    if (swap <= 0) return true; // even losing our piece right back is fine

    Bitboard occ = board.occ() ^ Bitboard::fromSquare(from) ^ Bitboard::fromSquare(to);
    Bitboard atk = attackers_to(board, to, occ);
    const Bitboard bishops = board.pieces(PieceType::BISHOP) | board.pieces(PieceType::QUEEN);
    const Bitboard rooks = board.pieces(PieceType::ROOK) | board.pieces(PieceType::QUEEN);

    Color stm = board.at(from).color();
    int res = 1;

    while (true)
    {
        stm = ~stm;
        atk &= occ;
        Bitboard mine = atk & board.us(stm);
        if (!mine) break;
        res ^= 1;

        Bitboard bb;
        if ((bb = mine & board.pieces(PieceType::PAWN)))
        {
            if ((swap = see_value(PieceType::PAWN) - swap) < res) break;
            occ ^= Bitboard::fromSquare(bb.lsb());
            atk |= attacks::bishop(to, occ) & bishops;
        }
        else if ((bb = mine & board.pieces(PieceType::KNIGHT)))
        {
            if ((swap = see_value(PieceType::KNIGHT) - swap) < res) break;
            occ ^= Bitboard::fromSquare(bb.lsb());
        }
        else if ((bb = mine & board.pieces(PieceType::BISHOP)))
        {
            if ((swap = see_value(PieceType::BISHOP) - swap) < res) break;
            occ ^= Bitboard::fromSquare(bb.lsb());
            atk |= attacks::bishop(to, occ) & bishops;
        }
        else if ((bb = mine & board.pieces(PieceType::ROOK)))
        {
            if ((swap = see_value(PieceType::ROOK) - swap) < res) break;
            occ ^= Bitboard::fromSquare(bb.lsb());
            atk |= attacks::rook(to, occ) & rooks;
        }
        else if ((bb = mine & board.pieces(PieceType::QUEEN)))
        {
            if ((swap = see_value(PieceType::QUEEN) - swap) < res) break;
            occ ^= Bitboard::fromSquare(bb.lsb());
            atk |= (attacks::bishop(to, occ) & bishops) | (attacks::rook(to, occ) & rooks);
        }
        else // king: can only recapture if the opponent has no attacker left
            return (atk & ~board.us(stm)) ? res ^ 1 : res;
    }
    return res != 0;
}

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
        int mvv_lva = piece_values[victim] - piece_values[attacker] / 100;
        // Captures that lose material (SEE < 0) go after all quiet moves.
        // Their score is negative, which qsearch uses to skip them.
        return see_ge(board, move, 0) ? SCORE_CAPTURE + mvv_lva : SCORE_BAD_CAPTURE + mvv_lva;
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
