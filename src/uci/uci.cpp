#include <sstream>
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <algorithm>
#include <cstdlib>

#include "uci.h"
#include "chess.hpp"
#include "engine/search.h"
#include "engine/polyglot.h"
#include "engine/syzygy.h"
#include "engine/movesort.h"
#include "engine/tt.h"

#include <filesystem>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace chess;

// The search runs on its own thread, so both threads may print at the same
// time: the mutex makes sure lines never get mixed together.
static std::mutex output_mutex;

void uci_send(const std::string &line)
{
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << line << "\n" << std::flush;
}

void parsePosition(Board &board, std::istringstream &iss)
{
    std::string token;
    iss >> token;

    if (token == "startpos")
    {
        board = Board();
        iss >> token; // "moves" (or nothing)
    }
    else if (token == "fen")
    {
        // Read FEN fields until "moves" (some GUIs send only 4 fields)
        std::string fen;
        while (iss >> token && token != "moves")
            fen += token + " ";
        board = Board(fen);
    }

    while (iss >> token)
    {
        if (token == "moves") continue;
        Move m = uci::uciToMove(board, token);
        if (m == Move::NO_MOVE) break; // invalid move: ignore the rest
        board.makeMove(m);
    }
}

// "go wtime 60000 btime 60000 winc 1000 binc 1000 movestogo 40 ..."
static SearchLimits parseGo(const Board &board, std::istringstream &iss)
{
    SearchLimits lim;
    const bool white = board.sideToMove() == Color::WHITE;
    std::string token;

    while (iss >> token)
    {
        if (token == "infinite")
        {
            lim.infinite = true;
            continue;
        }

        int64_t value;
        if (!(iss >> value)) break;

        if (token == "depth") lim.depth = static_cast<int>(value);
        else if (token == "movetime") lim.movetime = value;
        else if (token == "movestogo") lim.movestogo = static_cast<int>(value);
        else if (token == (white ? "wtime" : "btime")) lim.time = value;
        else if (token == (white ? "winc" : "binc")) lim.inc = value;
    }
    return lim;
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
        uci_send("info string loaded opening book " + bookFile);
    else
        uci_send("info string no opening book loaded (" + bookFile + ")");
}

static void loadSyzygy()
{
    if (syzygy_init(syzygyPath))
        uci_send("info string loaded syzygy tablebases up to " +
                 std::to_string(syzygy_max_pieces()) + " pieces from " + syzygyPath);
    else
        uci_send("info string no syzygy tablebases loaded (" + syzygyPath + ")");
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
    else if (name == "Hash")
    {
        int mb = std::clamp(std::atoi(value.c_str()), 1, 4096);
        tt.resize(static_cast<size_t>(mb));
    }
}

// --- Search thread ----------------------------------------------------------
static std::thread search_thread;

static void waitForSearch()
{
    if (search_thread.joinable())
        search_thread.join();
}

static void stopSearch()
{
    stop_search = true;
    waitForSearch();
}

static void startSearch(const Board &board, const SearchLimits &lim)
{
    stopSearch(); // never run two searches at once
    stop_search = false;

    search_thread = std::thread([board, lim]()
    {
        SearchResult result = search(board, lim);

        if (result.best_move == Move::NO_MOVE)
        {
            uci_send("bestmove 0000"); // no legal move (mate / stalemate)
            return;
        }
        if (std::string(result.source) != "search")
            uci_send(std::string("info string ") + result.source + " move");

        uci_send("bestmove " + uci::moveToUci(result.best_move));
    });
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
            uci_send("id name Argus");
            uci_send("id author Aitaneuh");
            uci_send("option name Hash type spin default 64 min 1 max 4096");
            uci_send("option name OwnBook type check default true");
            uci_send("option name BookFile type string default " + bookFile);
            uci_send("option name SyzygyPath type string default " + syzygyPath);
            uci_send("uciok");
        }
        else if (token == "isready")
        {
            uci_send("readyok"); // answers even while searching
        }
        else if (token == "setoption")
        {
            stopSearch();
            parseSetOption(iss);
        }
        else if (token == "ucinewgame")
        {
            stopSearch();
            board = Board();
            tt.clear();
            clear_move_ordering();
        }
        else if (token == "position")
        {
            stopSearch();
            parsePosition(board, iss);
        }
        else if (token == "go")
        {
            SearchLimits lim = parseGo(board, iss);
            startSearch(board, lim);
        }
        else if (token == "stop")
        {
            stopSearch();
        }
        else if (token == "quit")
        {
            break;
        }
    }

    stopSearch();
}
