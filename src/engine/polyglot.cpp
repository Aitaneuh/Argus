#include "polyglot.h"
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>

using namespace chess;

extern const uint64_t Random64[781];

struct BookEntry { uint64_t key; uint16_t move; uint16_t weight; };
static std::vector<BookEntry> book;

static uint64_t be(const unsigned char *p, int n)
{
    uint64_t v = 0;
    for (int i = 0; i < n; i++) v = (v << 8) | p[i];
    return v;
}

bool polyglot_load(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    unsigned char buf[16];
    book.clear();
    while (f.read(reinterpret_cast<char *>(buf), 16))
        book.push_back({be(buf, 8), (uint16_t)be(buf + 8, 2), (uint16_t)be(buf + 10, 2)});
    // file is already sorted by key (required by the format)
    return !book.empty();
}

uint64_t polyglot_key(const Board &board)
{
    uint64_t key = 0;

    for (int sq = 0; sq < 64; sq++)
    {
        Piece p = board.at(Square(sq));
        if (p == Piece::NONE) continue;
        int type = static_cast<int>(p.type().internal()); // PAWN=0 ... KING=5
        int kind = 2 * type + (p.color() == Color::WHITE ? 1 : 0);
        key ^= Random64[64 * kind + sq]; // sq = 8*row + file, same as polyglot
    }

    auto cr = board.castlingRights();
    if (cr.has(Color::WHITE, Board::CastlingRights::Side::KING_SIDE))  key ^= Random64[768];
    if (cr.has(Color::WHITE, Board::CastlingRights::Side::QUEEN_SIDE)) key ^= Random64[769];
    if (cr.has(Color::BLACK, Board::CastlingRights::Side::KING_SIDE))  key ^= Random64[770];
    if (cr.has(Color::BLACK, Board::CastlingRights::Side::QUEEN_SIDE)) key ^= Random64[771];

    // En passant: only counts if a pawn can actually capture
    Square ep = board.enpassantSq();
    if (ep != Square::NO_SQ)
    {
        Color stm = board.sideToMove();
        if (attacks::pawn(~stm, ep) & board.pieces(PieceType::PAWN, stm))
            key ^= Random64[772 + ep.file()];
    }

    if (board.sideToMove() == Color::WHITE) key ^= Random64[780];
    return key;
}

static uint16_t encode(const Move &m)
{
    int from = m.from().index(), to = m.to().index();
    int promo = 0;
    if (m.typeOf() == Move::PROMOTION)
        promo = static_cast<int>(m.promotionType().internal()); // N=1,B=2,R=3,Q=4 matches polyglot
    // castling in chess.hpp is already "king takes own rook", same as polyglot
    return (to & 7) | ((to >> 3) << 3) | ((from & 7) << 6) | ((from >> 3) << 9) | (promo << 12);
}

std::optional<Move> polyglot_pick(const Board &board)
{
    if (book.empty()) return std::nullopt;
    uint64_t key = polyglot_key(board);

    auto it = std::lower_bound(book.begin(), book.end(), key,
        [](const BookEntry &e, uint64_t k) { return e.key < k; });

    Movelist legal;
    movegen::legalmoves(legal, board);

    std::vector<std::pair<Move, int>> cands;
    for (; it != book.end() && it->key == key; ++it)
        for (const Move &m : legal)
            if (encode(m) == it->move && it->weight > 0)
            {
                cands.push_back({m, it->weight});
                break;
            }

    if (cands.empty()) return std::nullopt;

    // Variety: the raw weights are "how often GMs played it", so the main line
    // would be picked almost every time. Instead:
    //  - drop rare moves (less than 1/16 of the most popular one), they are often dubious
    //  - pick among the rest with sqrt(weight), which flattens the odds
    int maxWeight = 0;
    for (auto &c : cands) maxWeight = std::max(maxWeight, c.second);

    std::vector<Move> moves;
    std::vector<double> odds;
    for (auto &[m, w] : cands)
        if (w * 16 >= maxWeight)
        {
            moves.push_back(m);
            odds.push_back(std::sqrt(static_cast<double>(w)));
        }

    // Seed from both random_device and the clock, so every game differs
    static std::mt19937 rng{std::random_device{}() ^
                            static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())};
    std::discrete_distribution<size_t> pick(odds.begin(), odds.end());
    return moves[pick(rng)];
}