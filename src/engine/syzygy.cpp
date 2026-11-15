#include "syzygy.h"
#include "tbprobe.h"

using namespace chess;

// Fathom's tb_init() wants ';' separated folders on Windows and ':' elsewhere.
bool syzygy_init(const std::string &path)
{
    if (path.empty() || path == "<empty>")
    {
        tb_free();
        return false;
    }
    if (!tb_init(path.c_str()))
        return false;
    return TB_LARGEST > 0;
}

int syzygy_max_pieces() { return static_cast<int>(TB_LARGEST); }

// Converts a WDL value (TB_LOSS .. TB_WIN) into an engine score.
static int wdl_to_score(unsigned wdl)
{
    switch (wdl)
    {
    case TB_WIN:  return TB_WIN_SCORE;
    case TB_LOSS: return -TB_WIN_SCORE;
    default:      return 0; // draws, and wins/losses spoiled by the 50-move rule
    }
}

static bool probeable(const Board &board)
{
    if (TB_LARGEST == 0) return false;
    if (board.occ().count() > static_cast<int>(TB_LARGEST)) return false;
    auto cr = board.castlingRights();
    return !cr.has(Color::WHITE) && !cr.has(Color::BLACK);
}

static unsigned ep_square(const Board &board)
{
    Square ep = board.enpassantSq();
    return ep == Square::NO_SQ ? 0 : static_cast<unsigned>(ep.index());
}

// Bitboards in the order Fathom expects them.
#define TB_BITBOARDS(b)                                   \
    (b).us(Color::WHITE).getBits(),                       \
    (b).us(Color::BLACK).getBits(),                       \
    (b).pieces(PieceType::KING).getBits(),                \
    (b).pieces(PieceType::QUEEN).getBits(),               \
    (b).pieces(PieceType::ROOK).getBits(),                \
    (b).pieces(PieceType::BISHOP).getBits(),              \
    (b).pieces(PieceType::KNIGHT).getBits(),              \
    (b).pieces(PieceType::PAWN).getBits()

std::optional<int> syzygy_probe_wdl(const Board &board)
{
    // WDL tables are only exact right after a capture or pawn move
    // (half-move clock 0), so Fathom refuses other positions.
    if (!probeable(board) || board.halfMoveClock() != 0) return std::nullopt;

    unsigned res = tb_probe_wdl(TB_BITBOARDS(board), 0, 0, ep_square(board),
                                board.sideToMove() == Color::WHITE);
    if (res == TB_RESULT_FAILED) return std::nullopt;
    return wdl_to_score(res);
}

std::optional<TBRootResult> syzygy_probe_root(const Board &board)
{
    if (!probeable(board)) return std::nullopt;

    unsigned res = tb_probe_root(TB_BITBOARDS(board), board.halfMoveClock(), 0,
                                 ep_square(board), board.sideToMove() == Color::WHITE, nullptr);
    if (res == TB_RESULT_FAILED || res == TB_RESULT_CHECKMATE || res == TB_RESULT_STALEMATE)
        return std::nullopt;

    int from = TB_GET_FROM(res), to = TB_GET_TO(res);
    PieceType promo = PieceType::NONE;
    switch (TB_GET_PROMOTES(res))
    {
    case TB_PROMOTES_QUEEN:  promo = PieceType::QUEEN; break;
    case TB_PROMOTES_ROOK:   promo = PieceType::ROOK; break;
    case TB_PROMOTES_BISHOP: promo = PieceType::BISHOP; break;
    case TB_PROMOTES_KNIGHT: promo = PieceType::KNIGHT; break;
    }

    // Find the matching legal move so it carries the right move type.
    Movelist legal;
    movegen::legalmoves(legal, board);
    for (const Move &m : legal)
    {
        if (m.from().index() != from || m.to().index() != to) continue;
        bool isPromo = m.typeOf() == Move::PROMOTION;
        if (isPromo != (promo != PieceType::NONE)) continue;
        if (isPromo && m.promotionType() != promo) continue;

        // Prefer quicker wins / slower losses using the DTZ value.
        int score = wdl_to_score(TB_GET_WDL(res));
        int dtz = TB_GET_DTZ(res);
        if (score > 0) score -= dtz;
        if (score < 0) score += dtz;
        return TBRootResult{m, score};
    }
    return std::nullopt;
}
