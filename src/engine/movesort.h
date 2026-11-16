#pragma once
#include "chess.hpp"
#include <cstdint>

constexpr int MAX_PLY = 128;

// Forget killers / history (call at the start of a new game).
void clear_move_ordering();

// Sort moves best-first:
//   1. the move stored in the transposition table (best move from earlier search)
//   2. captures, most valuable victim / least valuable attacker first (MVV-LVA)
//   3. queen promotions
//   4. killer moves (quiet moves that caused a cutoff at the same ply)
//   5. other quiet moves, by history score
void sort_moves(chess::Movelist &moves, const chess::Board &board,
                uint16_t tt_move = 0, int ply = 0);

// Static Exchange Evaluation: true if playing `move` and then trading off every
// piece attacking the target square (cheapest first) wins at least `threshold`
// centipawns for the side to move. see_ge(m, 0) = "this capture doesn't lose material".
bool see_ge(const chess::Board &board, const chess::Move &move, int threshold);

// Rough piece value used by SEE and delta pruning (P N B R Q K NONE).
int see_value(chess::PieceType pt);

// Called when a quiet move causes a beta cutoff, so it gets tried earlier next time.
void record_quiet_cutoff(const chess::Board &board, const chess::Move &move, int ply, int depth);
