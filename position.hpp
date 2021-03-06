//========================================================================
// position.hpp
// 2012.9.8-2019.9.4
//========================================================================
#pragma once

#include <cstdio>
#include <algorithm>
#include <numeric>
#include <array>
#include "move.hpp"
#include "color.hpp"

constexpr unsigned MaxBreadth = 250; //real limit should be smaller

struct position{ // positon
	std::array<unsigned,81> Board; // also the inverted index for coordinates
	std::array<unsigned,20> Coord; // coordinates for marbles; blacks 1st
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
	unsigned ListMoves(move Moves[MaxBreadth], const int bar = -16) const;
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
			Coord[black++] = b;
			Score[0] += Scores[b];
			Hash ^= Hashes[b][0];
		}
		else if (board[b] == 'w') {
			Board[b] = white;
			Coord[white++] = b;
			Score[1] += Scores[b];
			Hash ^= Hashes[b][1];
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
			printf("%02d", (i - j) * 9 + j);
            if (b < 20)
                SetConsoleColor(Color::reset);
            printf("  ");
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

	Coord[Board[m.orig]] = m.dest;
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

	Coord[Board[m.dest]] = m.orig;
	Board[m.orig] = Board[m.dest];
	Board[m.dest] = ' ';
}

// list all possible moves of current position
unsigned position::ListMoves(move Moves[MaxBreadth], const int bar) const
{
	move Naive[MaxBreadth];
	unsigned MovesNo = 0;

	unsigned ChainIds[81] = {}; // the id of the chain each cell belongs to
	unsigned LastChain = 1; // the 0th chain is not used
	unsigned Chains[81]; // all the chains; chain i starts from ChainStarts[i]
	unsigned ChainStarts[24] = {0, 0}; // I bet 23 chains are enough

    // sort the Coord by distance from home; the distant coord comes first
    // since the counting sort later is stable, if two moves have the same
    // score, then the move with distant orig will be listed first
	std::array<unsigned, 10> SortedCoord;
	if (WhiteTurn) {
		std::copy(Coord.cbegin() + 10, Coord.cend(), SortedCoord.begin());
        constexpr auto f = [](auto x, auto y) { return Scores[x] < Scores[y]; };
        std::sort(SortedCoord.begin(), SortedCoord.end(), f);
	}
	else {
		std::copy(Coord.cbegin(), Coord.cbegin() + 10, SortedCoord.begin());
        constexpr auto f = [](auto x, auto y) { return Scores[x] > Scores[y]; };
        std::sort(SortedCoord.begin(), SortedCoord.end(), f);
	}
	for (const auto orig : SortedCoord) {
		if (bar < 2) { // adjacent moves will never have >=2 score
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
					for (unsigned j = ChainStarts[ChainId];
						j < ChainStarts[ChainId + 1]; j++) {
						const unsigned c = Chains[j];
						Naive[MovesNo++].Set(orig, c);
						visited[c] = true;
					}
				}
				else { // find the new chain
					unsigned Rear = MovesNo; // prepare the queue of BFS
					Naive[MovesNo++].Set(orig, dest);
					visited[dest] = true;
					ChainIds[dest] = LastChain;
					unsigned ChainStart = ChainStarts[LastChain];
					Chains[ChainStart++] = dest;
					while (Rear != MovesNo) {
						const unsigned mid = Naive[Rear++].dest;
						for (unsigned k = 0; k < Next[mid].HopNo; k++) {
							const unsigned adj = Next[mid].Adj[k];
							const unsigned dest = Next[mid].Hop[k];
							if (IsMarble(adj) && IsSpace(dest)
								&& !visited[dest]) {
								Naive[MovesNo++].Set(orig, dest);
								visited[dest] = true;
								ChainIds[dest] = LastChain;
								Chains[ChainStart++] = dest;
							}
						}
					}
					ChainStarts[++LastChain] = ChainStart;
				}
			}
		}
	}
	if (MovesNo > 129) {
		printf("MovesNo=%d\n", MovesNo);
		Print();
		getchar();
	}

	// scores are negated because the counting sort below is ascending
	int MoveScores[MaxBreadth];
	unsigned j = 0;
	const int NegatedSign = WhiteTurn ? -1 : 1;
	for (unsigned i = 0; i < MovesNo; i++) {
		const int NegatedScore = Naive[i].Score() * NegatedSign;
		if (NegatedScore <= -bar) { // Score >= bar
			Naive[j] = Naive[i];
			MoveScores[j++] = NegatedScore + 16; // make all scores >= 0
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
