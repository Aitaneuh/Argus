#include "search.h"
#include "movesort.h"
#include "evaluate.h"
#include "chess.hpp"

using namespace chess;

const int INF = 99999999;

int minimax(Board &board, int depth, bool white_to_play, int alpha, int beta)
{
    Movelist legal_moves;
    movegen::legalmoves(legal_moves, board);

    if (legal_moves.size() == 0)
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

    Movelist sorted_moves = sort_moves(legal_moves);

    if (white_to_play)
    {
        int best_eval = -INF;
        for (int i = 0; i < sorted_moves.size(); i++)
        {
            board.makeMove(sorted_moves[i]);
            int eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(sorted_moves[i]);

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
        for (int i = 0; i < sorted_moves.size(); i++)
        {
            board.makeMove(sorted_moves[i]);
            int eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(sorted_moves[i]);

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

Move search(Board &board, int depth)
{
    Movelist legal_moves;
    movegen::legalmoves(legal_moves, board);

    Move best_move = legal_moves[0];
    bool white_to_play = board.sideToMove() == Color::WHITE;

    int alpha = -INF;
    int beta = INF;

    if (white_to_play)
    {
        int best_eval = -INF;
        for (int i = 0; i < legal_moves.size(); i++)
        {
            board.makeMove(legal_moves[i]);
            int eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(legal_moves[i]);

            if (eval > best_eval)
            {
                best_eval = eval;
                best_move = legal_moves[i];
            }
            if (best_eval > alpha)
                alpha = best_eval;
        }
    }
    else
    {
        int best_eval = INF;
        for (int i = 0; i < legal_moves.size(); i++)
        {
            board.makeMove(legal_moves[i]);
            int eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(legal_moves[i]);

            if (eval < best_eval)
            {
                best_eval = eval;
                best_move = legal_moves[i];
            }
            if (best_eval < beta)
                beta = best_eval;
        }
    }

    return best_move;
}