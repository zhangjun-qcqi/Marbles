//========================================================================
// move.hpp
// 2012.9.8-2018.6.11
//========================================================================
#pragma once

#include <cstdio>
#include <bitset>
#include <climits>

int Scores[81];
typedef std::bitset<163> hash;
hash Hashes[81][2];
hash WhiteHash;
struct {
	unsigned AdjNo;
	unsigned Adj[6]; // first with hops, then adj-only; thus AdjNo >= HopNo
	unsigned HopNo;
	unsigned Hop[6];
} Next[81];

// precompute the globals
void PreCompute()
{
	constexpr int offsets[6][2] = {
		{ 0,-1 },//left
		{ -1,0 },//up
		{ 1,-1 },//down left
		{ -1,1 },//up right
		{ 1,0 },//down
		{ 0,1 },//right
	};
	WhiteHash.set(162);
	for (unsigned b = 0; b < 81; b++) {
		const unsigned i = b / 9;// Let's see if M$VC optimizes it to div
		const unsigned j = b % 9;
		Scores[b] = 8 - (i + j);// based on white's view; middle line is 0
		unsigned Adj[6];
		unsigned AdjNo = 0;
		for (unsigned k = 0; k < 6; k++) {
			const unsigned i2 = i + offsets[k][0];// note the unsigned wrap
			const unsigned j2 = j + offsets[k][1];
			if (i2 < 9 && j2 < 9) {
				const unsigned i3 = i2 + offsets[k][0];
				const unsigned j3 = j2 + offsets[k][1];
				if (i3 < 9 && j3 < 9) {
					Next[b].Adj[Next[b].AdjNo++] = i2 * 9 + j2;
					Next[b].Hop[Next[b].HopNo++] = i3 * 9 + j3;
				}
				else {
					Adj[AdjNo++] = i2 * 9 + j2;
				}
			}
		}
		for (unsigned k = 0; k < AdjNo; k++) {
			Next[b].Adj[Next[b].AdjNo++] = Adj[k];
		}
		Hashes[b][0].set(b * 2);
		Hashes[b][1].set(b * 2 + 1);
	}
}

void hash2ulls(const hash& h, unsigned long long ulls[3])
{
	constexpr hash mask = ULLONG_MAX;
	ulls[0] = (h >> 128 & mask).to_ullong();
	ulls[1] = (h >> 64 & mask).to_ullong();
	ulls[2] = (h & mask).to_ullong();
}

hash ulls2hash(const unsigned long long ulls[3])
{
	hash h = ulls[0];
	h <<= 64;
	h ^= ulls[1];
	h <<= 64;
	h ^= ulls[2];
	return h;
}

struct move{
	unsigned orig,dest;

	void Set(const unsigned o, const unsigned d) { orig = o; dest = d; }
	int Score() const { return Scores[dest] - Scores[orig];}
	int NegaScore() const { return Scores[orig] - Scores[dest];}
	void Set(const char b[]) {
		Set((b[0] - '0') * 10 + b[1] - '0', (b[2] - '0') * 10 + b[3] - '0');
	}
	void Print() const { printf("%02d%02d\n", orig, dest); }
	bool operator==(const move& b) const {
		return orig == b.orig && dest == b.dest;
	}
	bool operator!=(const move& b) const { return !operator==(b); }
};

move NullMove;
