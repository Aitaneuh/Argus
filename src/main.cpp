#include "chess.hpp"

using namespace chess;
using namespace std;

int main()
{
    Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    Movelist moves;
    movegen::legalmoves(moves, board);

    for (const auto &move : moves)
    {
        std::cout << uci::moveToUci(move) << std::endl;
    }
    cin.get();
    return 0;
}