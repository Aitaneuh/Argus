#include <sstream>
#include <iostream>
#include <string>

#include "uci.h"
#include "chess.hpp"
#include "engine/search.h"

using namespace chess;

void parsePosition(Board &board, std::istringstream &iss)
{
    std::string token;
    iss >> token;

    if (token == "startpos")
    {
        board = Board();
        iss >> token;
    }
    else if (token == "fen")
    {
        std::string fen;
        // read 6 FEN fields
        for (int i = 0; i < 6; i++)
        {
            iss >> token;
            fen += token + " ";
        }
        board = Board(fen);
        iss >> token;
    }

    while (iss >> token)
    {
        Move m = uci::uciToMove(board, token);
        board.makeMove(m);
    }
}

void uciLoop()
{
    Board board;
    std::string line, token;

    while (std::getline(std::cin, line))
    {
        std::istringstream iss(line);
        iss >> token;

        if (token == "uci")
        {
            std::cout << "id name Argus\n";
            std::cout << "id author Aitaneuh\n";
            std::cout << "uciok\n";
        }
        else if (token == "isready")
        {
            std::cout << "readyok\n";
        }
        else if (token == "ucinewgame")
        {
            board = Board();
        }
        else if (token == "position")
        {
            parsePosition(board, iss);
        }
        else if (token == "go")
        {
            int depth = 6;

            Movelist legal_moves;
            movegen::legalmoves(legal_moves, board);

            if (legal_moves.size() == 0)
            {
                std::cout << "bestmove 0000\n"; // UCI way to say no move
            }
            else
            {
                SearchResult result = search(board, depth);

                std::cout << "info depth " << depth
                          << " score cp " << result.score_cp
                          << " nodes " << result.nodes
                          << " nps " << result.nps
                          << "\n";

                std::cout << "bestmove " << uci::moveToUci(result.best_move) << "\n";
            }
        }
        else if (token == "quit")
        {
            break;
        }

        std::cout.flush();
    }
}
