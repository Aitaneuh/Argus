#include "search.h"
#include "movesort.h"
#include "evaluate.h"
#include "tt.h"
#include "chess.hpp"
#include "polyglot.h"
#include "syzygy.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <thread>

using namespace chess;
using Clock = std::chrono::steady_clock;

bool use_book = true;
std::atomic<bool> stop_search{false};

static constexpr int INF = 1000000;

// --- Search state (one search at a time) -----------------------------------
static uint64_t nodes;
static Clock::time_point start_time;
static int64_t hard_limit_ms; // abort the search after this (-1 = no limit)
static bool time_up;          // set once we must stop: unwinds the whole search
static bool depth1_done;      // never abort before we have at least one move
static Move root_best;        // best root move of the current iteration

static int64_t elapsed_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time).count();
}

// Checking the clock is slow-ish, so only do it every 2048 nodes.
static bool should_stop()
{
    if (!time_up && (nodes & 2047) == 0)
    {
        if (stop_search.load(std::memory_order_relaxed))
            time_up = true;
        else if (depth1_done && hard_limit_ms >= 0 && elapsed_ms() >= hard_limit_ms)
            time_up = true;
    }
    return time_up;
}

// Evaluation from the side to move's point of view (what negamax needs).
static int eval_stm(Board &board)
{
    int score = evaluate(board);
    return board.sideToMove() == Color::WHITE ? score : -score;
}

// Mate/tablebase scores depend on the distance to the root ("mate in 3 from
// here"). In the TT we store them relative to the current node instead, so
// the entry stays correct when the same position is reached at another ply.
static constexpr int WIN_BOUND = TB_WIN_SCORE - 1000;

static int score_to_tt(int score, int ply)
{
    if (score > WIN_BOUND) return score + ply;
    if (score < -WIN_BOUND) return score - ply;
    return score;
}

static int score_from_tt(int score, int ply)
{
    if (score > WIN_BOUND) return score - ply;
    if (score < -WIN_BOUND) return score + ply;
    return score;
}

static bool has_non_pawn_material(const Board &board)
{
    uint64_t own = board.us(board.sideToMove()).getBits();
    uint64_t pawnsKings = (board.pieces(PieceType::PAWN) | board.pieces(PieceType::KING)).getBits();
    return (own & ~pawnsKings) != 0;
}

// ---------------------------------------------------------------------------
// Quiescence search: at the end of the normal search we keep looking at
// captures until the position is "quiet". Without this the engine would stop
// in the middle of an exchange (e.g. right after QxP, not seeing PxQ) and
// badly misjudge the position: the "horizon effect".
// ---------------------------------------------------------------------------
static int qsearch(Board &board, int alpha, int beta, int ply)
{
    nodes++;
    if (should_stop()) return 0;

    int stand_pat = eval_stm(board);
    if (ply >= MAX_PLY - 1) return stand_pat;

    // "Stand pat": we are not forced to capture, so the static eval is a
    // lower bound of what we can get here.
    if (stand_pat >= beta) return stand_pat;
    if (stand_pat > alpha) alpha = stand_pat;

    Movelist moves;
    movegen::legalmoves<movegen::MoveGenType::CAPTURE>(moves, board);
    sort_moves(moves, board, 0, MAX_PLY);

    int best = stand_pat;
    for (const Move &move : moves)
    {
        board.makeMove(move);
        int score = -qsearch(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move);

        if (time_up) return 0;

        if (score > best)
        {
            best = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) break;
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// Main search: negamax + alpha-beta, with
//  - transposition table (cutoffs + best move ordering)
//  - check extension, null move pruning, late move reductions (LMR)
//  - principal variation search (PVS): after the first move, the others are
//    searched with a zero window first, which is cheaper when they are worse
// ---------------------------------------------------------------------------
static int negamax(Board &board, int depth, int alpha, int beta, int ply, bool allow_null)
{
    const bool root = (ply == 0);

    if (!root)
    {
        // Draws: 50-move rule, insufficient material, repetition. A position
        // seen once before is treated as a draw (the opponent can repeat it).
        if (board.isHalfMoveDraw() || board.isInsufficientMaterial() || board.isRepetition(1))
            return 0;

        // Mate distance pruning: no point searching for a mate longer than
        // one we already found.
        alpha = std::max(alpha, -MATE_SCORE + ply);
        beta = std::min(beta, MATE_SCORE - ply - 1);
        if (alpha >= beta) return alpha;
    }

    const bool in_check = board.inCheck();
    if (in_check) depth++; // check extension: never stop the search while in check

    if (depth <= 0) return qsearch(board, alpha, beta, ply);

    nodes++;
    if (should_stop()) return 0;
    if (ply >= MAX_PLY - 1) return eval_stm(board);

    // --- Transposition table probe ---
    const uint64_t key = board.hash();
    uint16_t tt_move = 0;
    if (TTEntry *entry = tt.probe(key))
    {
        tt_move = entry->move;
        if (!root && entry->depth >= depth)
        {
            int score = score_from_tt(entry->score, ply);
            if (entry->flag == TT_EXACT) return score;
            if (entry->flag == TT_BETA && score >= beta) return score;
            if (entry->flag == TT_ALPHA && score <= alpha) return score;
        }
    }

    // --- Syzygy endgame tablebase probe ---
    if (!root)
    {
        if (auto tb = syzygy_probe_wdl(board))
        {
            int score = *tb > 0 ? TB_WIN_SCORE - ply : *tb < 0 ? -TB_WIN_SCORE + ply : 0;
            tt.store(key, score_to_tt(score, ply), 0, MAX_PLY - 1, TT_EXACT);
            return score;
        }
    }

    // --- Null move pruning ---
    // If we skip our turn and the opponent still can't bring the score below
    // beta, our position is so good that a real move will also fail high.
    // Not done in check or with only pawns left (zugzwang danger).
    if (allow_null && !root && !in_check && depth >= 3 && std::abs(beta) < WIN_BOUND &&
        has_non_pawn_material(board) && eval_stm(board) >= beta)
    {
        int R = depth >= 7 ? 3 : 2;
        board.makeNullMove();
        int score = -negamax(board, depth - 1 - R, -beta, -beta + 1, ply + 1, false);
        board.unmakeNullMove();

        if (time_up) return 0;
        if (score >= beta) return score > WIN_BOUND ? beta : score;
    }

    Movelist moves;
    movegen::legalmoves(moves, board);
    if (moves.empty())
        return in_check ? -MATE_SCORE + ply : 0; // checkmate or stalemate

    sort_moves(moves, board, tt_move, ply);

    const int orig_alpha = alpha;
    int best = -INF;
    uint16_t best_move = 0;

    for (int i = 0; i < moves.size(); i++)
    {
        const Move move = moves[i];
        const bool quiet = !board.isCapture(move) && move.typeOf() != Move::PROMOTION;

        board.makeMove(move);
        int score;

        if (i == 0)
        {
            // First move (usually the TT/best move): full window
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
        }
        else
        {
            // Late move reductions: quiet moves sorted late are rarely good,
            // so search them less deep first and re-search if they surprise us.
            int reduction = 0;
            if (depth >= 3 && i >= 3 && quiet && !in_check && !board.inCheck())
                reduction = (i >= 8 && depth >= 6) ? 2 : 1;

            score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, true);
            if (score > alpha && reduction > 0)
                score = -negamax(board, depth - 1, -alpha - 1, -alpha, ply + 1, true);
            if (score > alpha && score < beta)
                score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, true);
        }

        board.unmakeMove(move);

        if (time_up) return 0;

        if (score > best)
        {
            best = score;
            best_move = move.move();
            if (root) root_best = move;

            if (score > alpha) alpha = score;
            if (alpha >= beta)
            {
                if (quiet) record_quiet_cutoff(board, move, ply, depth);
                break;
            }
        }
    }

    TTFlag flag = best >= beta ? TT_BETA : best > orig_alpha ? TT_EXACT : TT_ALPHA;
    tt.store(key, score_to_tt(best, ply), best_move, depth, flag);
    return best;
}

// ---------------------------------------------------------------------------
// Time management
// ---------------------------------------------------------------------------
// soft: don't start a new iteration after this much time.
// hard: abort the search in the middle of an iteration after this.
static void allocate_time(const SearchLimits &lim, int64_t &soft, int64_t &hard)
{
    const int64_t overhead = 30; // GUI / OS lag safety margin
    soft = hard = -1;

    if (lim.movetime > 0)
    {
        soft = hard = std::max<int64_t>(1, lim.movetime - overhead);
        return;
    }
    if (lim.time < 0) return; // no clock: depth limited or infinite

    const int64_t avail = std::max<int64_t>(1, lim.time - overhead);
    const int mtg = lim.movestogo > 0 ? std::min(lim.movestogo, 50) : 30;

    // Our "fair share" of the remaining time, plus most of the increment
    int64_t target = avail / mtg + lim.inc * 3 / 4;
    target = std::min(target, avail * 8 / 10);

    hard = std::max<int64_t>(1, std::min(target * 3, avail / 3));
    soft = std::min(target * 6 / 10, hard);
}

// Follows the best moves stored in the TT to build the principal variation.
static std::string pv_string(Board board, const Move &first, int max_len)
{
    std::string pv = uci::moveToUci(first);
    board.makeMove(first);

    for (int i = 1; i < max_len; i++)
    {
        TTEntry *e = tt.probe(board.hash());
        if (!e || e->move == 0) break;

        Movelist legal;
        movegen::legalmoves(legal, board);
        auto it = std::find_if(legal.begin(), legal.end(),
                               [&](const Move &m) { return m.move() == e->move; });
        if (it == legal.end()) break;

        pv += " " + uci::moveToUci(*it);
        board.makeMove(*it);
        if (board.isRepetition(1)) break;
    }
    return pv;
}

static std::string score_string(int score)
{
    if (score > MATE_BOUND) return "mate " + std::to_string((MATE_SCORE - score + 1) / 2);
    if (score < -MATE_BOUND) return "mate " + std::to_string(-(MATE_SCORE + score) / 2);
    return "cp " + std::to_string(score);
}

// ---------------------------------------------------------------------------
// Iterative deepening: search depth 1, 2, 3, ... until the time runs out.
// Every iteration fills the TT, which makes the next one much faster thanks
// to better move ordering, so this costs almost nothing extra.
// ---------------------------------------------------------------------------
SearchResult search(Board board, const SearchLimits &lim)
{
    start_time = Clock::now();
    nodes = 0;
    time_up = false;
    depth1_done = false;

    Movelist legal;
    movegen::legalmoves(legal, board);
    SearchResult result{legal.empty() ? Move(Move::NO_MOVE) : legal[0], 0, 0, "search"};
    if (legal.empty()) return result;

    // 1) Opening book
    if (use_book && !lim.infinite)
        if (auto bm = polyglot_pick(board))
            return {*bm, 0, 0, "book"};

    // 2) Endgame tablebase: perfect play when few pieces are left
    if (!lim.infinite)
        if (auto tb = syzygy_probe_root(board))
        {
            uci_send("info depth 1 score " + score_string(tb->score) + " pv " + uci::moveToUci(tb->move));
            return {tb->move, tb->score, 1, "syzygy"};
        }

    // 3) Normal search
    int64_t soft_limit;
    allocate_time(lim, soft_limit, hard_limit_ms);

    int max_depth = MAX_PLY - 10;
    if (lim.depth > 0)
        max_depth = std::min(lim.depth, max_depth);
    else if (lim.time < 0 && lim.movetime < 0 && !lim.infinite)
        max_depth = 8; // plain "go" with no limits (handy when typing by hand)

    for (int depth = 1; depth <= max_depth; depth++)
    {
        root_best = Move(Move::NO_MOVE);
        int score = negamax(board, depth, -INF, INF, 0, false);

        // An interrupted iteration is incomplete: keep the previous result.
        if (time_up && depth > 1) break;

        if (root_best != Move::NO_MOVE)
        {
            result.best_move = root_best;
            result.score = score;
            result.depth = depth;
        }
        depth1_done = true;

        int64_t ms = elapsed_ms();
        std::ostringstream info;
        info << "info depth " << depth << " score " << score_string(score)
             << " nodes " << nodes << " nps " << (ms > 0 ? nodes * 1000 / ms : 0)
             << " time " << ms << " pv " << pv_string(board, result.best_move, depth);
        uci_send(info.str());

        if (time_up) break;
        if (soft_limit >= 0 && ms >= soft_limit) break;

        // Found a forced mate shorter than the depth searched: can't do better.
        if (!lim.infinite && std::abs(score) > MATE_BOUND && MATE_SCORE - std::abs(score) < depth)
            break;
    }

    // UCI rule: with "go infinite" we must wait for "stop" before answering.
    while (lim.infinite && !stop_search.load())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    return result;
}
