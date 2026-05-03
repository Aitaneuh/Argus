#pragma once
#include <cstdint>
#include <vector>

enum TTFlag : uint8_t
{
    TT_NONE = 0,
    TT_EXACT = 1, // PV node — exact score
    TT_ALPHA = 2, // All node — upper bound (failed low)
    TT_BETA = 3   // Cut node — lower bound (beta cutoff)
};

struct TTEntry
{
    uint64_t key = 0;
    int32_t score = 0;
    uint16_t move = 0; // encoded move (cast from chess::Move)
    uint8_t depth = 0;
    TTFlag flag = TT_NONE;
};

class TranspositionTable
{
public:
    explicit TranspositionTable(size_t mb = 64);

    void clear();
    void store(uint64_t key, int score, uint16_t move, int depth, TTFlag flag);
    TTEntry *probe(uint64_t key);

private:
    std::vector<TTEntry> entries;
    size_t numEntries;
};

// Global instance — included by search.cpp
extern TranspositionTable tt;