//========================================================================
// position.hpp
// 2012.9.8-2018.3.30
//========================================================================
#pragma once

#include <cstdio>
#include <algorithm>
#include <numeric>
#include "move.hpp"
#include "color.hpp"

constexpr unsigned MaxBreadth = 100;

struct position{ // positon
	unsigned Board[81]; // also the inverted index for cordinates
	unsigned Coordinate[20]; // cordinates for the marbles; blacks first
	bool WhiteTurn; // true if it is white's turn
	int Score[2]; // scores for black and white
	unsigned long long Hash; // Zobrist hashing

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
	bool IsLegal(const char turn,const char board[]);
	void Print(); 
	void MakeMove(const move& m);
	void UndoMove(const move& m);
	unsigned ListMoves(move Moves[MaxBreadth], unsigned Index[MaxBreadth],
		const bool quiet =false);
};

// set current position using inputs
void position::Set(const char turn, const char board[])
{
	WhiteTurn = turn == 'w';
	unsigned white = 10;
	unsigned black = 0;
	Score[0] = 0;
	Score[1] = 0;
	Hash = 0;
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
bool position::IsLegal(const char turn, const char board[])
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
void position::Print()
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
}

// apply the move on current position
void position::MakeMove(const move& m)
{
	Score[WhiteTurn] += m.score;
	Hash ^= Hashes[m.dest][WhiteTurn];
	Hash ^= Hashes[m.orig][WhiteTurn];
	WhiteTurn = !WhiteTurn;
	Coordinate[Board[m.orig]] = m.dest;
	Board[m.dest] = Board[m.orig];
	Board[m.orig] = ' ';

}

// undo the move on current position
void position::UndoMove(const move& m)
{
	Score[WhiteTurn] -= m.score;
	WhiteTurn = !WhiteTurn;
	Coordinate[Board[m.dest]] = m.orig;
	Board[m.orig] = Board[m.dest];
	Board[m.dest] = ' ';
	Hash ^= Hashes[m.dest][WhiteTurn];
	Hash ^= Hashes[m.orig][WhiteTurn];
}

// list all possible moves of current position
unsigned position::ListMoves(move Moves[MaxBreadth],
	unsigned Index[MaxBreadth], const bool quiet)
{
	unsigned MovesNo = 0;
	unsigned start = WhiteTurn * 10;
	for (unsigned i = start; i < start + 10; i++) {
		unsigned orig = Coordinate[i];
		if (!quiet) { // only in quiescent search
			// first list the adjacent moves
			for (unsigned k = 0; k < Next[orig].AdjNo; k++) {
				const unsigned dest = Next[orig].Adj[k];
				if (IsSpace(dest))
					Moves[MovesNo++].Set(orig, dest);
			}
		}

		unsigned rear = MovesNo; // prepare the queue
		bool visited[81] = {};

		// then list the one hop jumps
		for (unsigned k = 0; k < Next[orig].HopNo; k++) {
			const unsigned adj = Next[orig].HopAdj[k];
			const unsigned dest = Next[orig].Hop[k];
			if (IsMarble(adj) && IsSpace(dest)) {
				Moves[MovesNo++].Set(orig, dest);
				visited[dest] = true;
			}
		}

		// last loops all the multiple hop jumps
		while (rear != MovesNo) {
			const unsigned mid = Moves[rear].dest;
			for (unsigned k = 0; k < Next[mid].HopNo; k++) {
				const unsigned adj = Next[mid].HopAdj[k];
				const unsigned dest = Next[mid].Hop[k];
				if (IsMarble(adj) && IsSpace(dest) && !visited[dest]) {
					Moves[MovesNo++].Set(orig, dest);
					visited[dest] = true;
				}
			}
			rear++;
		}
	}
	if(MovesNo >= MaxBreadth){
		printf("%d\n",MovesNo);
		Print();
		exit(0);
	}

	// counting sort the index
	unsigned count[33] = {};
	unsigned* const count2 = count + 16;
	for (unsigned i = 0; i < MovesNo; i++)
		count2[Moves[i].score]++;
	std::partial_sum(count, count + 33, count);
	for (unsigned i = MovesNo-1; i < MovesNo; i--)
		Index[--count2[Moves[i].score]] = i;
	if (WhiteTurn) // default ascending, so reverse the moves in white's turn
		std::reverse(Index, Index + MovesNo);
	if (quiet) // drop <2 moves in quiescent search; drop >-2 moves for black
		MovesNo = WhiteTurn ? MovesNo - count2[2] : count2[-1];
	return MovesNo;
}
