//========================================================================
// position.hpp
// 2012.9.8-2018.6.13
//========================================================================
#pragma once

#include <cstdio>
#include <algorithm>
#include <numeric>
#include <array>
#include "move.hpp"
#include "color.hpp"

constexpr unsigned MaxBreadth = 250;

struct position{ // positon
	std::array<unsigned,81> Board; // also the inverted index for coordinates
	std::array<unsigned,20> Coordinate; // coordinates for marbles; blacks 1st
	bool WhiteTurn; // true if it is white's turn
	std::array<int,2> Score; // scores for black and white
	hash Hash; // Zobrist hashing

	constexpr static const char* init =
		"bbbb     "
		"bbb      "
		"bb       "
		"b        "
		"         "
		"        w"
		"       ww"
		"      www"
		"     wwww";

	void Init(){Set('w',init);}
	void Set(const char turn,const char board[]);
	bool IsSpace(const unsigned b) const {return Board[b] == ' ';}
	bool IsMarble(const unsigned b) const {return Board[b] != ' ';}
	bool IsLegal(const char turn,const char board[]) const;
	void Print() const;
	void MakeMove(const move& m);
	void UndoMove(const move& m);
	unsigned ListMoves(move Moves[MaxBreadth], const int bar = 0) const;
	bool operator== (const position& b) const;
	bool operator!= (const position& b) const { return !operator==(b);}
};

// set current position using inputs
void position::Set(const char turn, const char board[])
{
	WhiteTurn = turn == 'w';
	unsigned white = 10;
	unsigned black = 0;
	Score[0] = 0;
	Score[1] = 0;
	Hash = WhiteTurn ? WhiteHash : 0;
	for (unsigned b = 0; b<81; b++) {
		if (board[b] == 'b') {
			Board[b] = black;
			Coordinate[black++] = b;
			Score[0] += Scores[b];
			Hash ^= Hashes[b][0];
		}
		else if (board[b] == 'w') {
			Board[b] = white;
			Coordinate[white++] = b;
			Score[1] += Scores[b];
			Hash ^= Hashes[b][0];
		}
		else
			Board[b] = ' ';
	}
}

// is input position legal?
bool position::IsLegal(const char turn, const char board[]) const
{
	if (turn != 'b'&&turn != 'w')
		return false;
	auto w = std::count(board, board + 81, 'w');
	auto b = std::count(board, board + 81, 'b');
	auto s = std::count(board, board + 81, ' ');
	if(w!=10||b!=10||w+b+s!=81)
		return false;
	return true;
}

// print current position
void position::Print() const
{
	for(int i=0;i<17;i++){
		for (int j = 0; j < std::max(8 - i,i-8); j++) printf("  ");
		for (int j = std::max(0,i-8); j <= std::min(i,8); j++) {
			unsigned b = Board[(i - j) * 9 + j];
			if(b<10)
				SetConsoleColor(Color::green);
			else if(b<20)
				SetConsoleColor(Color::red);
			printf("%02d  ", (i - j) * 9 + j);
			SetConsoleColor(Color::white);
		}
		printf("\n");
	}
	unsigned long long ulls[3];
	hash2ulls(Hash, ulls);
	printf("%d [%d %d] %llX-%llX-%llX\n",
		WhiteTurn, Score[0], Score[1], ulls[0], ulls[1], ulls[2]);
}

// apply the move on current position
void position::MakeMove(const move& m)
{
	Score[WhiteTurn] += m.Score();
	Hash ^= Hashes[m.dest][WhiteTurn];
	Hash ^= Hashes[m.orig][WhiteTurn];
	WhiteTurn = !WhiteTurn;
	Hash ^= WhiteHash;

	Coordinate[Board[m.orig]] = m.dest;
	Board[m.dest] = Board[m.orig];
	Board[m.orig] = ' ';
}

// undo the move on current position
void position::UndoMove(const move& m)
{
	Hash ^= WhiteHash;
	WhiteTurn = !WhiteTurn;
	Score[WhiteTurn] -= m.Score();
	Hash ^= Hashes[m.dest][WhiteTurn];
	Hash ^= Hashes[m.orig][WhiteTurn];

	Coordinate[Board[m.dest]] = m.orig;
	Board[m.orig] = Board[m.dest];
	Board[m.dest] = ' ';
}

// list all possible moves of current position
unsigned position::ListMoves(move Moves[MaxBreadth], const int bar) const
{
	move Naive[MaxBreadth];
	unsigned MovesNo = 0;

	unsigned ChainIds[81] = {};
	unsigned LastChain = 1; // the 0th chain is not used
	unsigned Chains[20][20];
	unsigned ChainNos[20] = {};

	unsigned start = WhiteTurn * 10;
	for (unsigned i = start; i < start + 10; i++) {
		const unsigned orig = Coordinate[i];
		if (bar < 2) { // only in quiescent search
			// first list the adjacent moves
			for (unsigned k = 0; k < Next[orig].AdjNo; k++) {
				const unsigned dest = Next[orig].Adj[k];
				if (IsSpace(dest))
					Naive[MovesNo++].Set(orig, dest);
			}
		}
		// then list the hop jumps
		bool visited[81] = {};
		for (unsigned k = 0; k < Next[orig].HopNo; k++) {
			const unsigned adj = Next[orig].Adj[k];
			const unsigned dest = Next[orig].Hop[k];
			if (IsMarble(adj) && IsSpace(dest) && !visited[dest]) {
				const unsigned ChainId = ChainIds[dest];
				if (ChainId != 0) { // already in the chains
					for (unsigned j = 0; j < ChainNos[ChainId]; j++) {
						const unsigned c = Chains[ChainId][j];
						Naive[MovesNo++].Set(orig, c);
						visited[c] = true;
					}
				}
				else { // find the new chain
					unsigned Rear = MovesNo; // prepare the queue
					Naive[MovesNo++].Set(orig, dest);
					visited[dest] = true;
					ChainIds[dest] = LastChain;
					unsigned ChainNo = 0;
					Chains[LastChain][ChainNo++] = dest;
					while (Rear != MovesNo) {
						const unsigned mid = Naive[Rear].dest;
						for (unsigned k = 0; k < Next[mid].HopNo; k++) {
							const unsigned adj = Next[mid].Adj[k];
							const unsigned dest = Next[mid].Hop[k];
							if (IsMarble(adj) && IsSpace(dest)
								&& !visited[dest]) {
								Naive[MovesNo++].Set(orig, dest);
								visited[dest] = true;
								ChainIds[dest] = LastChain;
								Chains[LastChain][ChainNo++] = dest;
							}
						}
						Rear++;
					}
					ChainNos[LastChain++] = ChainNo;
				}
			}
		}
	}
	if (MovesNo > 129) {
		printf("MovesNo=%d\n", MovesNo);
		Print();
		getchar();
	}

	int MoveScores[MaxBreadth];
	unsigned j = 0;
	const int sign = WhiteTurn ? -1 : 1;
	for (unsigned i = 0; i < MovesNo; i++) {
		const int s = Naive[i].Score() * sign;
		if (s <= -bar) { // drop <bar moves for white; >-bar moves for black
			Naive[j] = Naive[i];
			MoveScores[j++] = s + 16; // shift the scores to be all >= 0
		}
	}
	MovesNo = j;

	// counting sort the index
	unsigned count[33] = {};
	for (unsigned i = 0; i < MovesNo; i++)
		count[MoveScores[i]]++;
	// now count[i] = count of i
	std::partial_sum(count, count + 33, count);
	// now count[i] = first index of i+1
	for (int i = MovesNo - 1; i >= 0; i--)
		Moves[--count[MoveScores[i]]] = Naive[i];
	return MovesNo;
}

bool position::operator==(const position & b) const
{
	return Hash == b.Hash
		&& Score == b.Score
		&& WhiteTurn == b.WhiteTurn
		&& Board == b.Board;
}
