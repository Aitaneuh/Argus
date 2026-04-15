#include "search.h"
#include "chess.hpp"
#include <random>

using namespace chess;

Move search(Board board, Movelist legal_moves)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, legal_moves.size() - 1);
    Move best = legal_moves[dist(rng)];
    return best;
}