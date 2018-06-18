//========================================================================
// position.hpp
// 2012.9.8-2018.6.17
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
	unsigned ChainIds[81]; // the chain id of each cell
	unsigned Chains[81][20]; // chains
	unsigned ChainSizes[81]; // size of each chains
	unsigned EmptyChainSlots[81]; // unused chain slots
	unsigned EmptyChainNo;

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
	void Place(const unsigned b);
	void Take(const unsigned b);
	void Merge(const unsigned c1, const unsigned c2);
	void Split(const unsigned b);
	void Split2(const unsigned c1, const unsigned c2);
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
	for (unsigned b = 0; b < 81; b++) {
		ChainIds[b] = b;
		Chains[b][0] = b;
		ChainSizes[b] = 1;
		Board[b] = ' ';
	}
	EmptyChainNo = 0;
	for (unsigned b = 0; b<81; b++) {
		if (board[b] == 'b') {
			Board[b] = black;
			Coordinate[black++] = b;
			Score[0] += Scores[b];
			Hash ^= Hashes[b][0];
			Place(b);
		}
		else if (board[b] == 'w') {
			Board[b] = white;
			Coordinate[white++] = b;
			Score[1] += Scores[b];
			Hash ^= Hashes[b][1];
			Place(b);
		}
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
#ifndef NDEBUG
	for (unsigned b = 0; b < 81; b++) {
		if (IsSpace(b)) {
			if (ChainSizes[ChainIds[b]] > 1) {
				printf("%u: ", b);
				for (unsigned i = 0; i < ChainSizes[ChainIds[b]]; i++)
					printf("%u ", Chains[ChainIds[b]][i]);
				printf("\n");
			}
			else {
				const unsigned c = Chains[ChainIds[b]][0];
				if (c != b) {
					SetConsoleColor(Color::red);
					printf("%u: %u\n", b, c);
					SetConsoleColor(Color::white);
				}
			}
		}
	}
#endif
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
	auto b = Board[m.orig];
	Board[m.orig] = ' ';
	Take(m.orig);
	//Print();
	Board[m.dest] = b;
	Place(m.dest);
	//Print();
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
	auto b = Board[m.dest];
	Board[m.dest] = ' ';
	Take(m.dest);
	//Print();
	Board[m.orig] = b;
	Place(m.orig);
	//Print();
}

// list all possible moves of current position
unsigned position::ListMoves(move Moves[MaxBreadth], const int bar) const
{
	move Naive[MaxBreadth];
	unsigned MovesNo = 0;
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
		bool Visited[81] = {};
		for (unsigned k = 0; k < Next[orig].HopNo; k++) {
			const unsigned Adj = Next[orig].Adj[k];
			const unsigned Dest = Next[orig].Hop[k];
			if (IsMarble(Adj) && IsSpace(Dest) && !Visited[Dest]) {
				const unsigned ChainId = ChainIds[Dest];
				for (unsigned j = 0; j < ChainSizes[ChainId]; j++) {
					const unsigned c = Chains[ChainId][j];
					Naive[MovesNo++].Set(orig, c);
					Visited[c] = true;
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

// place a marble on cell b
void position::Place(const unsigned b)
{
	for (unsigned k = 0; k < Next[b].ConjNo; k += 2) {
		const unsigned Conj1 = Next[b].Conj[k];
		const unsigned Conj2 = Next[b].Conj[k + 1];
		if (IsSpace(Conj1) && IsSpace(Conj2))
			Merge(Conj1, Conj2);
	}

	Split(b);
}

// take the marble from cell b
void position::Take(const unsigned b)
{
	const unsigned ChainId = EmptyChainSlots[--EmptyChainNo];
	ChainIds[b] = ChainId;
	Chains[ChainId][0] = b;
	ChainSizes[ChainId] = 1;

	for (unsigned k = 0; k < Next[b].HopNo; k++) {
		const unsigned Adj = Next[b].Adj[k];
		const unsigned Dest = Next[b].Hop[k];
		if (IsMarble(Adj) && IsSpace(Dest))
			Merge(b, Dest);
	}

	for (unsigned k = 0; k < Next[b].ConjNo; k += 2) {
		const unsigned Conj1 = Next[b].Conj[k];
		const unsigned Conj2 = Next[b].Conj[k + 1];
		if (IsSpace(Conj1) && IsSpace(Conj2))
			Split2(Conj1, Conj2);
	}
}

// merge the chains of c1 and c2
void position::Merge(const unsigned c1, const unsigned c2)
{
	unsigned ChainId1 = ChainIds[c1];
	unsigned ChainId2 = ChainIds[c2];
	if (ChainId1 != ChainId2) { // ignore if already in the same chain
		if (ChainSizes[ChainId1] > ChainSizes[ChainId2])
			std::swap(ChainId1, ChainId2); // so that chain2 is shorter
		std::copy(Chains[ChainId2],
			Chains[ChainId2] + ChainSizes[ChainId2],
			Chains[ChainId1] + ChainSizes[ChainId1]);
		for (unsigned i = 0; i < ChainSizes[ChainId2]; i++)
			ChainIds[Chains[ChainId2][i]] = ChainId1;
		ChainSizes[ChainId1] += ChainSizes[ChainId2];
		ChainSizes[ChainId2] = 0;
		EmptyChainSlots[EmptyChainNo++] = ChainId2;
	}
}

// split the chain of b, ignoring b itself
void position::Split(const unsigned b)
{
	const unsigned ChainId = ChainIds[b];
	if (ChainSizes[ChainId] == 1) {
		EmptyChainSlots[EmptyChainNo++] = ChainId;
		ChainSizes[ChainId] = 0;
	}
	else if (ChainSizes[ChainId] == 2) {
		Chains[ChainId][0] = Chains[ChainId][0] + Chains[ChainId][1] - b;
		ChainSizes[ChainId] = 1;
	}
	else {
		bool Visited[81] = {};
		Visited[b] = true;
		for (unsigned i = 0; i < ChainSizes[ChainId]; i++) {
			const unsigned c = Chains[ChainId][i];
			if (Visited[c])
				continue;
			const unsigned EmptyChainId = EmptyChainSlots[--EmptyChainNo];
			unsigned ChainSize = 0;
			Chains[EmptyChainId][ChainSize++] = c;
			Visited[c] = true;
			unsigned Rear = 0;
			ChainIds[c] = EmptyChainId;
			while (Rear != ChainSize) {
				const unsigned c = Chains[EmptyChainId][Rear++];
				for (unsigned k = 0; k < Next[c].HopNo; k++) {
					const unsigned Adj = Next[c].Adj[k];
					const unsigned Dest = Next[c].Hop[k];
					if (IsMarble(Adj) && IsSpace(Dest) && !Visited[Dest]) {
						Chains[EmptyChainId][ChainSize++] = Dest;
						Visited[Dest] = true;
						ChainIds[Dest] = EmptyChainId;
					}
				}
			}
			ChainSizes[EmptyChainId] = ChainSize;
		}
		EmptyChainSlots[EmptyChainNo++] = ChainId;
		ChainSizes[ChainId] = 0;
	}
}


void position::Split2(const unsigned c1, const unsigned c2)
{
	const unsigned ChainId = ChainIds[c1];
	if (ChainSizes[ChainId] == 2) {
		const unsigned EmptyChainId = EmptyChainSlots[--EmptyChainNo];
		Chains[EmptyChainId][0] = Chains[ChainId][1];
		ChainIds[Chains[ChainId][1]] = EmptyChainId;
		ChainSizes[EmptyChainId] = 1;
		ChainSizes[ChainId] = 1;
	}
	else {
		bool Visited[81] = {};
		const unsigned temp[2] = { c1, c2 };
		for (auto c: temp) {
			if (Visited[c])
				continue;
			const unsigned EmptyChainId = EmptyChainSlots[--EmptyChainNo];
			unsigned ChainSize = 0;
			Chains[EmptyChainId][ChainSize++] = c;
			Visited[c] = true;
			unsigned Rear = 0;
			ChainIds[c] = EmptyChainId;
			while (Rear != ChainSize) {
				const unsigned c = Chains[EmptyChainId][Rear++];
				for (unsigned k = 0; k < Next[c].HopNo; k++) {
					const unsigned Adj = Next[c].Adj[k];
					const unsigned Dest = Next[c].Hop[k];
					if (IsMarble(Adj) && IsSpace(Dest) && !Visited[Dest]) {
						Chains[EmptyChainId][ChainSize++] = Dest;
						Visited[Dest] = true;
						ChainIds[Dest] = EmptyChainId;
					}
				}
			}
			ChainSizes[EmptyChainId] = ChainSize;
		}
		EmptyChainSlots[EmptyChainNo++] = ChainId;
		ChainSizes[ChainId] = 0;
	}
}
