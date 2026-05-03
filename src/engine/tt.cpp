#include "tt.h"
#include <algorithm>

// Define the global TT instance (64 MB by default)
TranspositionTable tt(64);

TranspositionTable::TranspositionTable(size_t mb)
{
    numEntries = (mb * 1024 * 1024) / sizeof(TTEntry);
    entries.resize(numEntries);
}

void TranspositionTable::clear()
{
    std::fill(entries.begin(), entries.end(), TTEntry{});
}

void TranspositionTable::store(uint64_t key, int score, uint16_t move, int depth, TTFlag flag)
{
    TTEntry &e = entries[key % numEntries];

    // Replacement policy: always replace if same position, or replace if new search is deeper
    if (e.key != key && e.depth > depth)
        return;

    e.key = key;
    e.score = score;
    e.move = move;
    e.depth = static_cast<uint8_t>(depth);
    e.flag = flag;
}

TTEntry *TranspositionTable::probe(uint64_t key)
{
    TTEntry *e = &entries[key % numEntries];
    return (e->key == key) ? e : nullptr;
}