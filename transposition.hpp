//========================================================================
// transposition.cpp
// 2018.4.9-2018.4.9
//========================================================================
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
	char Compressed[20]; // the compressed coordinates
	int Score;
	unsigned Depth;
	move Move;
	scoretype ScoreType;
};