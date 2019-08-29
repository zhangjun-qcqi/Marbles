//========================================================================
// transposition.hpp
// 2018.4.9-2018.8.29
//========================================================================
#pragma once

#include <cstdio>
#include <array>
#include "move.hpp"

constexpr int NoCutOff = 137; // also represents an impossible score

struct transposition{ // transposition
	int Lowerbound;
	int Upperbound;
	unsigned Depth;
    move Move; // the best move that has been found, otherwise NullMove
	unsigned Age; // the last ply that the entry has been effectively used

	void Print() const {
		printf("%u[%d %d]%u", Depth, Lowerbound, Upperbound, Age);
	}
};
