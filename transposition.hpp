//========================================================================
// transposition.cpp
// 2018.4.9-2018.4.17
//========================================================================
#pragma once

#include <cstdio>
#include "move.hpp"

constexpr int NoCutOff = 137; // also represents an impossible score

struct transposition{ // transposition
	int Lowerbound;
	int Upperbound;
	unsigned Depth;
	move Move;
	unsigned Age;

	void Print() const {
		printf("%u[%d %d]%u", Depth, Lowerbound, Upperbound, Age);
	}
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
constexpr unsigned mediumQuiets[] = { 4, 6, 8 };