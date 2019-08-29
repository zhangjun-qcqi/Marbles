//========================================================================
// config.hpp
// 2019.8.29-2019.8.29
//========================================================================
#pragma once

#include <array>

struct config { // pre-defined position and depths used for benchmark
    unsigned MaxDepth;
    unsigned TTableDepth;
    std::array<unsigned, 3> QuietDepths;
    char Board[82];
    char Player;
};

constexpr config Easy = {
    9, 8, {2, 4, 7},
    "bbbb     "
    " bb      "
    "         "
    " bb      "
    "  bbww   "
    "      ww "
    "       w "
    "      ww "
    "     w ww",
    'b'
};

constexpr config Medium = {
    11, 9, {4, 6, 8},
    "b        "
    "bb b     "
    "bb bb    "
    "b b      "
    "    ww   "
    "      ww "
    "       w "
    "      ww "
    "     w ww",
    'b'
};
