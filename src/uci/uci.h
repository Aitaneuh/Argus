#pragma once
#include <sstream>
#include <string>
#include <iostream>
#include "chess.hpp"

void parsePosition(chess::Board &board, std::istringstream &iss);
void uciLoop();