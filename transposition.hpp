//========================================================================
// transposition.cpp
// 2018.4.9-2018.4.17
//========================================================================
#pragma once

#include "move.hpp"

struct transposition{ // transposition
	int Lowerbound;
	int Upperbound;
	unsigned Depth;
	move Move;
};

const char* easy =
"bbbb     "
" bb      "
"         "
" bb      "
"  bbww   "
"      ww "
"       w "
"      ww "
"     w ww";
constexpr unsigned easyDepths[] = { 9, 8 };
constexpr unsigned easyQuiets[] = { 2, 4, 7 };

const char* medium =
"b        "
"bb b     "
"bb bb    "
"b b      "
"    ww   "
"      ww "
"       w "
"      ww "
"     w ww";
constexpr unsigned mediumDepths[] = { 11, 9 };
constexpr unsigned mediumQuiets[] = { 3, 5, 7 };