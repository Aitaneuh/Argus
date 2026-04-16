#include "search.h"
#include "movesort.h"
#include "evaluate.h"
#include "chess.hpp"
#include <random>

using namespace chess;

float minimax(Board board, int depth, bool white_to_play, float alpha, float beta)
{
    Square opposing_king_sq;
    Color ally_color;
    Color opponent_color;

    if (white_to_play)
    {
        ally_color = Color::WHITE;
        opponent_color = Color::BLACK;
    }
    else
    {
        ally_color = Color::BLACK;
        opponent_color = Color::WHITE;
    }

    Movelist legal_moves;
    movegen::legalmoves(legal_moves, board);

    // checkmate win
    if (board.isAttacked(board.kingSq(opponent_color), ally_color) and legal_moves.size() == 0)
    {
        return 100000 + depth;
    }

    // checkmate lose
    if (board.isAttacked(board.kingSq(ally_color), opponent_color) and legal_moves.size() == 0)
    {
        return -100000 - depth;
    }

    if (legal_moves.size() == 0 or depth == 0)
    {
        return evaluate(board);
    }

    Movelist sorted_moves = sort_moves(legal_moves);

    if (white_to_play)
    {
        float best_eval = -INFINITY;
        for (int i = 0; i < sorted_moves.size(); i++)
        {
            board.makeMove(sorted_moves[i]);
            float eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(sorted_moves[i]);

            if (eval > best_eval)
            {
                best_eval = eval;
            }
            if (best_eval > alpha)
            {
                alpha = best_eval;
            }
            if (beta <= alpha)
            {
                break;
            }
        }
        return best_eval;
    }
    else
    {
        float best_eval = INFINITY;
        for (int i = 0; i < sorted_moves.size(); i++)
        {
            board.makeMove(sorted_moves[i]);
            float eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(sorted_moves[i]);

            if (eval < best_eval)
            {
                best_eval = eval;
            }
            if (best_eval < beta)
            {
                beta = best_eval;
            }
            if (beta <= alpha)
            {
                break;
            }
        }
        return best_eval;
    }
}

Move search(Board board, Movelist legal_moves, int depth)
{
    Move best_move;

    Movelist sorted_moves = sort_moves(legal_moves);
    bool white_to_play = board.fullMoveNumber() % 2 == 0;

    float alpha = -INFINITY;
    float beta = INFINITY;

    if (white_to_play)
    {
        float best_eval = -INFINITY;
        for (int i = 0; i < sorted_moves.size(); i++)
        {
            board.makeMove(sorted_moves[i]);
            float eval = minimax(board, depth - 1, false, alpha, beta);
            board.unmakeMove(sorted_moves[i]);

            if (eval > best_eval)
            {
                best_eval = eval;
                best_move = sorted_moves[i];
            }
            if (best_eval > alpha)
            {
                alpha = best_eval;
            }
        }
    }
    else
    {
        float best_eval = INFINITY;
        for (int i = 0; i < sorted_moves.size(); i++)
        {
            board.makeMove(sorted_moves[i]);
            float eval = minimax(board, depth - 1, true, alpha, beta);
            board.unmakeMove(sorted_moves[i]);

            if (eval < best_eval)
            {
                best_eval = eval;
                best_move = sorted_moves[i];
            }
            if (best_eval < beta)
            {
                beta = best_eval;
            }
        }
    }

    return best_move;
}
