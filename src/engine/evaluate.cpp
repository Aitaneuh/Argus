#include "chess.hpp"
#include "evaluate.h"
#include <cstdint>

using namespace chess;

// ---------------------------------------------------------------------------
// Tapered evaluation with piece-square tables (PST).
//
// Every piece gets a bonus/malus depending on the square it stands on, and
// there are two sets of tables: one for the middlegame (mg) and one for the
// endgame (eg). The final score blends both depending on how much material
// is left ("game phase"): with all pieces on the board it is 100% mg, with
// only kings and pawns it is 100% eg. This is why e.g. the king wants to hide
// in the middlegame but walk to the center in the endgame.
//
// Values are the well-known PeSTO tables by Ronald Friederich
// (https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function).
//
// The tables are written "as you see the board" from White's side:
// the first row is rank 8, the last row is rank 1. chess.hpp uses a1 = 0,
// so for a white piece on square sq we read index sq ^ 56 (mirror the rank),
// and for a black piece we read index sq directly (Black sees it flipped).
// ---------------------------------------------------------------------------

//                           P    N    B    R     Q  K
static const int mg_value[6] = {82, 337, 365, 477, 1025, 0};
static const int eg_value[6] = {94, 281, 297, 512, 936, 0};

// How much each piece type counts towards the game phase (24 = opening).
static const int phase_inc[6] = {0, 1, 1, 2, 4, 0};

// clang-format off
static const int mg_pawn[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};
static const int eg_pawn[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};
static const int mg_knight[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};
static const int eg_knight[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};
static const int mg_bishop[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};
static const int eg_bishop[64] = {
    -14, -21, -11,  -8,  -7,  -9, -17, -24,
     -8,  -4,   7, -12,  -3, -13,  -4, -14,
      2,  -8,   0,  -1,  -2,   6,   0,   4,
     -3,   9,  12,   9,  14,  10,   3,   2,
     -6,   3,  13,  19,   7,  10,  -3,  -9,
    -12,  -3,   8,  10,  13,   3,  -7, -15,
    -14, -18,  -7,  -1,   4,  -9, -15, -27,
    -23,  -9, -23,  -5,  -9, -16,  -5, -17,
};
static const int mg_rook[64] = {
     32,  42,  32,  51,  63,   9,  31,  43,
     27,  32,  58,  62,  80,  67,  26,  44,
     -5,  19,  26,  36,  17,  45,  61,  16,
    -24, -11,   7,  26,  24,  35,  -8, -20,
    -36, -26, -12,  -1,   9,  -7,   6, -23,
    -45, -25, -16, -17,   3,   0,  -5, -33,
    -44, -16, -20,  -9,  -1,  11,  -6, -71,
    -19, -13,   1,  17,  16,   7, -37, -26,
};
static const int eg_rook[64] = {
     13,  10,  18,  15,  12,  12,   8,   5,
     11,  13,  13,  11,  -3,   3,   8,   3,
      7,   7,   7,   5,   4,  -3,  -5,  -3,
      4,   3,  13,   1,   2,   1,  -1,   2,
      3,   5,   8,   4,  -5,  -6,  -8, -11,
     -4,   0,  -5,  -1,  -7, -12,  -8, -16,
     -6,  -6,   0,   2,  -9,  -9, -11,  -3,
     -9,   2,   3,  -1,  -5, -13,   4, -20,
};
static const int mg_queen[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};
static const int eg_queen[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};
static const int mg_king[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};
static const int eg_king[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43,
};
// clang-format on

static const int *mg_table[6] = {mg_pawn, mg_knight, mg_bishop, mg_rook, mg_queen, mg_king};
static const int *eg_table[6] = {eg_pawn, eg_knight, eg_bishop, eg_rook, eg_queen, eg_king};

// ---------------------------------------------------------------------------
// Positional terms on top of the PST (v5).
//
// All values are {middlegame, endgame} pairs and go through the same tapered
// blend as the PST. They are hand-picked classic values (not tuned yet).
// ---------------------------------------------------------------------------

typedef std::uint64_t U64;

static inline int popcnt(U64 b) { return __builtin_popcountll(b); }
static inline int lsb(U64 b) { return __builtin_ctzll(b); }

static constexpr U64 FILE_A = 0x0101010101010101ULL;
static constexpr U64 FILE_H = FILE_A << 7;

static U64 FILE_BB[8];
static U64 ADJ_FILES[8];
static U64 PASSED_MASK[2][64]; // squares in front of a pawn (own + adjacent files)

static struct MaskInit
{
    MaskInit()
    {
        for (int f = 0; f < 8; f++)
            FILE_BB[f] = FILE_A << f;
        for (int f = 0; f < 8; f++)
            ADJ_FILES[f] = (f > 0 ? FILE_BB[f - 1] : 0) | (f < 7 ? FILE_BB[f + 1] : 0);
        for (int sq = 0; sq < 64; sq++)
        {
            int f = sq & 7, r = sq >> 3;
            U64 files = FILE_BB[f] | ADJ_FILES[f];
            U64 above = 0, below = 0;
            for (int rr = r + 1; rr < 8; rr++)
                above |= 0xFFULL << (8 * rr);
            for (int rr = 0; rr < r; rr++)
                below |= 0xFFULL << (8 * rr);
            PASSED_MASK[0][sq] = files & above;
            PASSED_MASK[1][sq] = files & below;
        }
    }
} mask_init;

static inline U64 pawn_attacks(U64 pawns, int c)
{
    if (c == 0)
        return ((pawns << 7) & ~FILE_H) | ((pawns << 9) & ~FILE_A);
    return ((pawns >> 9) & ~FILE_H) | ((pawns >> 7) & ~FILE_A);
}

// --- Pawn structure ---
static const int DOUBLED_MG = -10, DOUBLED_EG = -25;   // per extra pawn on a file
static const int ISOLATED_MG = -12, ISOLATED_EG = -15; // no friendly pawn on adjacent files
// Passed pawn bonus by relative rank. The PST already rewards advanced pawns
// a lot, so these stay moderate.
static const int PASSED_MG[8] = {0, 0, 5, 10, 20, 35, 60, 0};
static const int PASSED_EG[8] = {0, 5, 10, 20, 40, 70, 110, 0};

// --- Mobility: bonus per reachable square, centred on a "normal" count ---
//                            N  B  R  Q
static const int MOB_MG[4]   = {4, 5, 2, 1};
static const int MOB_EG[4]   = {4, 5, 4, 2};
static const int MOB_BASE[4] = {4, 6, 6, 12};

// --- King safety (middlegame only) ---
static const int SHIELD_1 = 12; // own pawn right in front of the king
static const int SHIELD_2 = 6;  // own pawn two squares in front
static const int SHIELD_MISSING = -14;
static const int OPEN_FILE_NEAR_KING = -12;       // no own pawn on a file next to the king
static const int ATTACK_WEIGHT[4] = {2, 2, 3, 5}; // N B R Q, per attacked king-zone square

int evaluate(Board &board)
{
    int mg[2] = {0, 0}; // [WHITE, BLACK]
    int eg[2] = {0, 0};
    int phase = 0;

    for (int pt = 0; pt < 6; pt++)
    {
        PieceType type = static_cast<PieceType::underlying>(pt);

        for (int c = 0; c < 2; c++)
        {
            Color color = static_cast<Color::underlying>(c);
            Bitboard bb = board.pieces(type, color);

            while (bb)
            {
                int sq = bb.pop();
                int idx = (color == Color::WHITE) ? (sq ^ 56) : sq;

                mg[c] += mg_value[pt] + mg_table[pt][idx];
                eg[c] += eg_value[pt] + eg_table[pt][idx];
                phase += phase_inc[pt];
            }
        }
    }

    const U64 occ = board.occ().getBits();
    U64 pawns[2], pawnAtt[2], own[2], kingZone[2];
    int kingSq[2];
    for (int c = 0; c < 2; c++)
    {
        Color color = static_cast<Color::underlying>(c);
        pawns[c] = board.pieces(PieceType::PAWN, color).getBits();
        pawnAtt[c] = pawn_attacks(pawns[c], c);
        own[c] = board.us(color).getBits();
        kingSq[c] = board.kingSq(color).index();
        kingZone[c] = attacks::king(Square(kingSq[c])).getBits() | (1ULL << kingSq[c]);
    }

    int attackWeight[2] = {0, 0}; // pressure by side c on the enemy king
    int attackers[2] = {0, 0};

    for (int c = 0; c < 2; c++)
    {
        const int them = c ^ 1;
        Color color = static_cast<Color::underlying>(c);

        // ---- Pawn structure ----
        for (int f = 0; f < 8; f++)
        {
            int n = popcnt(pawns[c] & FILE_BB[f]);
            if (n > 1)
            {
                mg[c] += DOUBLED_MG * (n - 1);
                eg[c] += DOUBLED_EG * (n - 1);
            }
            if (n && !(pawns[c] & ADJ_FILES[f]))
            {
                mg[c] += ISOLATED_MG * n;
                eg[c] += ISOLATED_EG * n;
            }
        }
        for (U64 b = pawns[c]; b; b &= b - 1)
        {
            int sq = lsb(b);
            if (!(PASSED_MASK[c][sq] & pawns[them]))
            {
                int rel = c == 0 ? (sq >> 3) : 7 - (sq >> 3);
                mg[c] += PASSED_MG[rel];
                eg[c] += PASSED_EG[rel];
            }
        }

        // ---- Mobility + attacks on the enemy king zone ----
        // Squares attacked by enemy pawns don't count as "mobile".
        const U64 area = ~own[c] & ~pawnAtt[them];
        for (int i = 0; i < 4; i++)
        {
            PieceType type = static_cast<PieceType::underlying>(i + 1); // N B R Q
            for (U64 b = board.pieces(type, color).getBits(); b; b &= b - 1)
            {
                Square sq(lsb(b));
                U64 att;
                switch (i)
                {
                case 0: att = attacks::knight(sq).getBits(); break;
                case 1: att = attacks::bishop(sq, occ).getBits(); break;
                case 2: att = attacks::rook(sq, occ).getBits(); break;
                default: att = attacks::queen(sq, occ).getBits(); break;
                }
                int mob = popcnt(att & area) - MOB_BASE[i];
                mg[c] += MOB_MG[i] * mob;
                eg[c] += MOB_EG[i] * mob;

                U64 hit = att & kingZone[them];
                if (hit)
                {
                    attackers[c]++;
                    attackWeight[c] += ATTACK_WEIGHT[i] * popcnt(hit);
                }
            }
        }

        // ---- King pawn shield / open files near the king (middlegame) ----
        const int kf = kingSq[c] & 7, kr = kingSq[c] >> 3;
        const int dir = c == 0 ? 1 : -1;
        for (int f = (kf > 0 ? kf - 1 : 0); f <= (kf < 7 ? kf + 1 : 7); f++)
        {
            int r1 = kr + dir, r2 = kr + 2 * dir;
            if (r1 >= 0 && r1 < 8 && (pawns[c] & (1ULL << (r1 * 8 + f))))
                mg[c] += SHIELD_1;
            else if (r2 >= 0 && r2 < 8 && (pawns[c] & (1ULL << (r2 * 8 + f))))
                mg[c] += SHIELD_2;
            else
                mg[c] += SHIELD_MISSING;

            if (!(pawns[c] & FILE_BB[f]))
                mg[c] += OPEN_FILE_NEAR_KING;
        }
    }

    // King attack: only counts with 2+ attackers and the queen still on.
    // Quadratic, so piling up pieces on the king hurts more and more.
    for (int c = 0; c < 2; c++)
    {
        Color color = static_cast<Color::underlying>(c);
        if (attackers[c] >= 2 && board.pieces(PieceType::QUEEN, color))
        {
            int w = attackWeight[c];
            int danger = w * w / 2;
            mg[c] += danger > 500 ? 500 : danger;
        }
    }

    int mgScore = mg[0] - mg[1];
    int egScore = eg[0] - eg[1];

    // Phase 24 = all pieces on board (pure middlegame), 0 = pure endgame.
    // Early promotions can push it above 24, so clamp it.
    int mgPhase = phase > 24 ? 24 : phase;
    int egPhase = 24 - mgPhase;

    // White's point of view (search flips it for Black).
    return (mgScore * mgPhase + egScore * egPhase) / 24;
}
