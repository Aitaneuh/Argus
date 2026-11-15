#pragma once
#include "chess.hpp"
#include <optional>
#include <string>

bool polyglot_load(const std::string &path);
uint64_t polyglot_key(const chess::Board &board);
std::optional<chess::Move> polyglot_pick(const chess::Board &board);