//========================================================================
// transposition.cpp
// 2018.4.9-2018.4.11
//========================================================================
#pragma once

#include <cstdio>
#include <algorithm>
#include <numeric>
#include "move.hpp"
#include "color.hpp"

enum scoretype{
	exact,
	lowerbound,
	upperbound
};

struct transposition{ // transposition
	int Score;
	scoretype ScoreType;
	unsigned Depth;
	move Move;
};