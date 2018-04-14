//========================================================================
// transposition.cpp
// 2018.4.9-2018.4.15
//========================================================================
#pragma once

#include <cstdio>
#include <algorithm>
#include <numeric>
#include "move.hpp"
#include "color.hpp"

struct transposition{ // transposition
	int Lowerbound;
	int Upperbound;
	unsigned Depth;
	move Move;
};