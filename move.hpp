//========================================================================
// move.hpp
// 2012.9.8-2016.10.13
//========================================================================
#pragma once

#include<cstdio>

int Rank[81];
struct {
	unsigned AdjNbr;
	unsigned Adj[6];
	unsigned HopNbr;
	unsigned HopAdj[6]; // the adj that can hop
	unsigned Hop[6];
} Net[81];


// pre-compute the globals
void Pre()
{
	constexpr int offsets[6][2] = {
		{ 0,-1 },//left
		{ -1,0 },//up
		{ 1,-1 },//down left
		{ -1,1 },//up right
		{ 1,0 },//down
		{ 0,1 },//right
	};
	for (unsigned b = 0; b < 81; b++) {
		const unsigned i = b / 9;// Let's see if M$VC optimizes it to div
		const unsigned j = b % 9;
		Rank[b] = 8 - (i + j);// based on white's view; middle line is 0
		for (unsigned k = 0; k < 6; k++) {
			const unsigned i2 = i + offsets[k][0];// note the unsigned wrap
			const unsigned j2 = j + offsets[k][1];
			if (i2 < 9 && j2 < 9) {
				Net[b].Adj[Net[b].AdjNbr++] = i2 * 9 + j2;
				const unsigned i3 = i2 + offsets[k][0];
				const unsigned j3 = j2 + offsets[k][1];
				if (i3 < 9 && j3 < 9) {
					Net[b].HopAdj[Net[b].HopNbr] = i2 * 9 + j2;
					Net[b].Hop[Net[b].HopNbr++] = i3 * 9 + j3;
				}
			}
		}
	}
}

struct move{
    unsigned orig,dest;
	int rank;

	void Set(const unsigned o, const unsigned d) {
		orig = o;
		dest = d;
		rank = Rank[dest] - Rank[orig];
	}
	void Set(const char b[]) {
		Set((b[0] - '0') * 10 + b[1] - '0', (b[2] - '0') * 10 + b[3] - '0');
	}
	void Print() { printf("%02d%02d\n", orig, dest); }
	bool operator==(const move& b) const {
		return orig == b.orig && dest == b.dest;
	}
	bool operator<(const move& b) const{
		return rank < b.rank;
	}
};
