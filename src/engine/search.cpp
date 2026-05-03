#include "search.h"
#include "movesort.h"
#include "evaluate.h"
#include "tt.h"
#include "chess.hpp"
#include <chrono>

using namespace chess;

const int INF = 99999999;
static int node_count = 0;

int negamax(Board &board, int depth, int alpha, int beta)
{
    node_count++;

    uint64_t key = board.hash();
    int origAlpha = alpha;

    // --- TT Probe ---
    TTEntry *entry = tt.probe(key);
    if (entry && entry->depth >= depth)
    {
        if (entry->flag == TT_EXACT)
            return entry->score;
        if (entry->flag == TT_ALPHA)
            beta = std::min(beta, entry->score);
        if (entry->flag == TT_BETA)
            alpha = std::max(alpha, entry->score);
        if (alpha >= beta)
            return entry->score;
    }

    if (depth == 0)
    {
        int score = evaluate(board);
        return board.sideToMove() == Color::WHITE ? score : -score;
    }

    Movelist moves;
    movegen::legalmoves(moves, board);

    if (moves.size() == 0)
        return board.inCheck() ? -(100000 + depth) : 0;

    sort_moves(moves, board);

    int bestScore = -INF;
    uint16_t bestMove = 0;

    for (int i = 0; i < moves.size(); i++)
    {
        board.makeMove(moves[i]);
        int score = -negamax(board, depth - 1, -beta, -alpha);
        board.unmakeMove(moves[i]);

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = static_cast<uint16_t>(moves[i].move());
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
        {
            tt.store(key, bestScore, bestMove, depth, TT_BETA);
            return bestScore;
        }
    }

    TTFlag flag = (bestScore <= origAlpha) ? TT_ALPHA : TT_EXACT;
    tt.store(key, bestScore, bestMove, depth, flag);

    return bestScore;
}

SearchResult search(Board &board, int depth)
{
    node_count = 0;

    auto start = std::chrono::high_resolution_clock::now();

    Movelist moves;
    movegen::legalmoves(moves, board);

    sort_moves(moves, board);

    Move best_move = moves[0];
    int best_eval = -INF;
    int alpha = -INF;
    int beta = INF;

    for (int i = 0; i < moves.size(); i++)
    {
        board.makeMove(moves[i]);
        int eval = -negamax(board, depth - 1, -beta, -alpha);
        board.unmakeMove(moves[i]);

        if (eval > best_eval)
        {
            best_eval = eval;
            best_move = moves[i];
        }
        if (eval > alpha)
            alpha = eval;
    }

    auto end = std::chrono::high_resolution_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int nps = elapsed_ms > 0 ? (node_count * 1000) / elapsed_ms : 0;

    return {best_move, best_eval, node_count, nps};
}