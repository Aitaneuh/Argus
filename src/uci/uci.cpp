#include <sstream>
#include <iostream>
#include <string>

#include "uci.h"
#include "chess.hpp"
#include "engine/search.h"
#include "engine/polyglot.h"
#include "engine/syzygy.h"

#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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

// Folder containing Argus.exe, so the default book / tablebases are found
// no matter which working directory cutechess starts the engine from.
static std::filesystem::path exeDir()
{
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH)
        return std::filesystem::path(buf).parent_path();
#endif
    return std::filesystem::current_path();
}

static std::string bookFile;
static std::string syzygyPath;

static void loadBook()
{
    if (polyglot_load(bookFile))
        std::cout << "info string loaded opening book " << bookFile << "\n";
    else
        std::cout << "info string no opening book loaded (" << bookFile << ")\n";
}

static void loadSyzygy()
{
    if (syzygy_init(syzygyPath))
        std::cout << "info string loaded syzygy tablebases up to "
                  << syzygy_max_pieces() << " pieces from " << syzygyPath << "\n";
    else
        std::cout << "info string no syzygy tablebases loaded (" << syzygyPath << ")\n";
}

// "setoption name <Name> value <Value>" (value may contain spaces)
static void parseSetOption(std::istringstream &iss)
{
    std::string token, name, value;
    iss >> token; // "name"
    while (iss >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;
    std::getline(iss, value);
    value.erase(0, value.find_first_not_of(' '));
    while (!value.empty() && (value.back() == ' ' || value.back() == '\r'))
        value.pop_back();

    if (name == "OwnBook")
        use_book = (value == "true");
    else if (name == "BookFile")
    {
        bookFile = value;
        loadBook();
    }
    else if (name == "SyzygyPath")
    {
        syzygyPath = value;
        loadSyzygy();
    }
}

void uciLoop()
{
    Board board;
    std::string line, token;

    bookFile = (exeDir() / "gm2001.bin").string();
    syzygyPath = (exeDir() / "syzygy").string();
    polyglot_load(bookFile);
    syzygy_init(syzygyPath);

    while (std::getline(std::cin, line))
    {
        std::istringstream iss(line);
        token.clear();
        iss >> token;

        if (token == "uci")
        {
            std::cout << "id name Argus\n";
            std::cout << "id author Aitaneuh\n";
            std::cout << "option name OwnBook type check default true\n";
            std::cout << "option name BookFile type string default " << bookFile << "\n";
            std::cout << "option name SyzygyPath type string default " << syzygyPath << "\n";
            std::cout << "uciok\n";
        }
        else if (token == "isready")
        {
            std::cout << "readyok\n";
        }
        else if (token == "setoption")
        {
            parseSetOption(iss);
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

                if (std::string(result.source) != "search")
                    std::cout << "info string " << result.source << " move\n";

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
