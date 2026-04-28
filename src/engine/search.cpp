#include "search.h"
#include "movesort.h"
#include "evaluate.h"
#include "chess.hpp"
#include <chrono>

using namespace chess;

const int INF = 99999999;
static int node_count = 0;

int minimax(Board &board, int depth, bool white_to_play, int alpha, int beta)
{
    node_count++;
    Movelist moves;
    movegen::legalmoves(moves, board);
    sort_moves(moves, board);

    if (moves.size() == 0)
    {
        if (board.inCheck())
            return white_to_play ? -100000 - depth : 100000 + depth;
        else
            return 0; // stalemate
    }

    if (depth == 0)
    {
        return evaluate(board);
    }

    if (white_to_play)
    {
        int best_eval = -INF;
        for (int i = 0; i < moves.size(); i++)
        {
            board.makeMove(moves[i]);
            int eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(moves[i]);

            if (eval > best_eval)
                best_eval = eval;
            if (best_eval > alpha)
                alpha = best_eval;
            if (beta <= alpha)
                break;
        }
        return best_eval;
    }
    else
    {
        int best_eval = INF;
        for (int i = 0; i < moves.size(); i++)
        {
            board.makeMove(moves[i]);
            int eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(moves[i]);

            if (eval < best_eval)
                best_eval = eval;
            if (best_eval < beta)
                beta = best_eval;
            if (beta <= alpha)
                break;
        }
        return best_eval;
    }
}

SearchResult search(Board &board, int depth)
{
    node_count = 0;
    auto start = std::chrono::high_resolution_clock::now();

    Movelist moves;
    movegen::legalmoves(moves, board);

    Move best_move = moves[0];
    bool white_to_play = board.sideToMove() == Color::WHITE;

    int alpha = -INF;
    int beta = INF;

    int best_eval = white_to_play ? -INF : INF;

    if (white_to_play)
    {
        for (int i = 0; i < moves.size(); i++)
        {
            board.makeMove(moves[i]);
            int eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(moves[i]);

            if (eval > best_eval)
            {
                best_eval = eval;
                best_move = moves[i];
            }
            if (best_eval > alpha)
                alpha = best_eval;
        }
    }
    else
    {
        for (int i = 0; i < moves.size(); i++)
        {
            board.makeMove(moves[i]);
            int eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(moves[i]);

            if (eval < best_eval)
            {
                best_eval = eval;
                best_move = moves[i];
            }
            if (best_eval < beta)
                beta = best_eval;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    int nps = elapsed_ms > 0 ? (node_count * 1000) / elapsed_ms : 0;

    return {best_move, best_eval, node_count, nps};
}