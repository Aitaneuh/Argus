#include "chess.hpp"
#include "evaluate.h"

using namespace chess;

float evaluate(Board board)
{
    Bitboard wp_bb = board.pieces(PieceType::PAWN, Color::WHITE);
    Bitboard bp_bb = board.pieces(PieceType::PAWN, Color::BLACK);
    Bitboard wn_bb = board.pieces(PieceType::KNIGHT, Color::WHITE);
    Bitboard bn_bb = board.pieces(PieceType::KNIGHT, Color::BLACK);
    Bitboard wb_bb = board.pieces(PieceType::BISHOP, Color::WHITE);
    Bitboard bb_bb = board.pieces(PieceType::BISHOP, Color::BLACK);
    Bitboard wr_bb = board.pieces(PieceType::ROOK, Color::WHITE);
    Bitboard br_bb = board.pieces(PieceType::ROOK, Color::BLACK);
    Bitboard wq_bb = board.pieces(PieceType::QUEEN, Color::WHITE);
    Bitboard bq_bb = board.pieces(PieceType::QUEEN, Color::BLACK);
    Bitboard wk_bb = board.pieces(PieceType::KING, Color::WHITE);
    Bitboard bk_bb = board.pieces(PieceType::KING, Color::BLACK);

    float score = 0;

    while (wp_bb.count() > 0)
    {
        score += 100;
        wp_bb &= wp_bb.count() - 1;
    }

    while (bp_bb.count() > 0)
    {
        score -= 100;
        bp_bb &= bp_bb.count() - 1;
    }

    while (wn_bb.count() > 0)
    {
        score += 320;
        wn_bb &= wn_bb.count() - 1;
    }

    while (bn_bb.count() > 0)
    {
        score -= 320;
        bn_bb &= bn_bb.count() - 1;
    }

    while (wb_bb.count() > 0)
    {
        score += 330;
        wb_bb &= wb_bb.count() - 1;
    }

    while (bb_bb.count() > 0)
    {
        score -= 330;
        bb_bb &= bb_bb.count() - 1;
    }

    while (wr_bb.count() > 0)
    {
        score += 500;
        wr_bb &= wr_bb.count() - 1;
    }

    while (br_bb.count() > 0)
    {
        score -= 500;
        br_bb &= br_bb.count() - 1;
    }

    while (wq_bb.count() > 0)
    {
        score += 900;
        wq_bb &= wq_bb.count() - 1;
    }

    while (bq_bb.count() > 0)
    {
        score -= 900;
        bq_bb &= bq_bb.count() - 1;
    }

    return score;
}