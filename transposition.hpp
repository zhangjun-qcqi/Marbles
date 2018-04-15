//========================================================================
// transposition.cpp
// 2018.4.9-2018.4.15
//========================================================================
#pragma once

#include "move.hpp"

struct transposition{ // transposition
	int Lowerbound;
	int Upperbound;
	unsigned Depth;
	move Move;
};

constexpr char* easy =
"bbbb     "
" bb      "
"         "
" bb      "
"  bbww   "
"      ww "
"       w "
"      ww "
"     w ww";
constexpr unsigned easyDepths[] = { 9, 7, 8 };

constexpr char* medium =
"b        "
"bb b     "
"bb bb    "
"b b      "
"    ww   "
"      ww "
"       w "
"      ww "
"     w ww";
constexpr unsigned mediumDepths[] = { 11, 7, 9 };