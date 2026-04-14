#include <sstream>
#include <iostream>
#include <string>
#include <random>

#include "uci.h"
#include "chess.hpp"
// #include "engine/search.h"
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
    std::mt19937 rng(std::random_device{}());

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
            int depth = 0, score_cp = 0, nodes = 0, nps = 0;
            Movelist move_list_uci;

            // std::cout << "info depth " << depth << " score cp " << score_cp << " nodes " << nodes << " nps " << nps << " pv " << move_list_uci << std::endl;

            Movelist legal_moves;
            movegen::legalmoves(legal_moves, board);

            // Move best = search(board);

            if (legal_moves.size() == 0)
            {
                std::cout << "bestmove 0000\n"; // UCI way to say no move
            }
            else
            {
                std::uniform_int_distribution<int> dist(0, legal_moves.size() - 1);
                Move best = legal_moves[dist(rng)];
                std::cout << "bestmove " << uci::moveToUci(best) << "\n";
            }
        }
        else if (token == "quit")
        {
            break;
        }

        std::cout.flush();
    }
}
