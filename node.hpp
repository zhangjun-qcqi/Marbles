//========================================================================
// node.hpp
// 2012.9.8-2016.10.13
//========================================================================
#pragma once

#include<cstdio>
#include<algorithm>
#include<numeric>
#include"move.hpp"
#define NOMINMAX
#include <windows.h>

constexpr unsigned MaxMoves = 240;

struct node{//positon
    unsigned Board[81];// also the inverted index for cordinates
	unsigned Cord[20]; // cordinates for the marbles; first black then white
	bool Turn; // in the name of white
	int rank[2];

	constexpr static char* init = 
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
	int sign() const {return Turn * 2 - 1;} // Let's see what will M$VC do
	bool Quest(const char turn,const char board[]);
    void Print(); 
    void MakeMove(const move& m);
	void UndoMove(const move& m);
	unsigned ListMoves(move Moves[MaxMoves], unsigned Order[MaxMoves],
		const bool quiescent =false);
};

// set current node using inputs
void node::Set(const char turn, const char board[])
{
	Turn = turn == 'w';
	unsigned white = 10;
	unsigned black = 0;
	rank[0] = 0;
	rank[1] = 0;
	for (unsigned b = 0; b<81; b++) {
		if (board[b] == 'b') {
			Board[b] = black;
			Cord[black++] = b;
			rank[0] += Rank[b];
		}
		else if (board[b] == 'w') {
			Board[b] = white;
			Cord[white++] = b;
			rank[1] += Rank[b];
		}
		else
			Board[b] = ' ';
	}
}

// is input node legal? Set to it if so
bool node::Quest(const char turn, const char board[])
{
	if (turn != 'b'&&turn != 'w')
		return false;
    unsigned w = std::count(board, board + 81, 'w');
    unsigned b = std::count(board, board + 81, 'b');
    unsigned s = std::count(board, board + 81, ' ');
    if(w!=10||b!=10||w+b+s!=81)
        return false;
    Set(turn,board);
    return true;
}

// print current node
void node::Print()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    for(int i=0;i<17;i++){
		for (int j = 0; j < std::max(8 - i,i-8); j++) printf("  ");
		for (int j = std::max(0,i-8); j <= std::min(i,8); j++) {
			unsigned b = Board[(i - j) * 9 + j];
			if(b<10)
			//case 'w': printf("\033[32m"); break;
				SetConsoleTextAttribute(hConsole, 10);
			else if(b<20)
			//case 'b': printf("\033[31m"); break;
				SetConsoleTextAttribute(hConsole, 12);
			printf("%02d  ", (i - j) * 9 + j);
			//printf("\033[37m");
			SetConsoleTextAttribute(hConsole, 15);
		}
        printf("\n");
    }
}

// apply the move on current node
void node::MakeMove(const move& m)
{
	rank[Turn] += m.rank;
	Turn = !Turn;
	Cord[Board[m.orig]] = m.dest;
	Board[m.dest] = Board[m.orig];
	Board[m.orig] = ' ';
}

// undo the move on current node
void node::UndoMove(const move& m)
{
	rank[Turn] -= m.rank;
	Turn = !Turn;
	Cord[Board[m.dest]] = m.orig;
	Board[m.orig] = Board[m.dest];
	Board[m.dest] = ' ';
}

// list all possible moves of current node
unsigned node::ListMoves(move Moves[MaxMoves], unsigned Order[MaxMoves],
	const bool quiescent)
{
	unsigned n = 0;
	unsigned start = Turn * 10;
	for (unsigned i = start; i < start + 10; i++) {
		unsigned b = Cord[i];
		if (!quiescent) { // only in quiescent search
			// first list the adjacent moves
			for (unsigned k = 0; k < AdjNbr[b]; k++) {
				const unsigned dest = Adj[b][k];
				if (IsSpace(dest))
					Moves[n++].Set(b, dest);
			}
		}

		unsigned rear = n; // prepare the queue
		bool visited[81] = {};

		// then list the one hop jumps
		for (unsigned k = 0; k < HopNbr[b]; k++) {
			const unsigned adj = HopAdj[b][k];
			const unsigned dest = Hop[b][k];
			if (IsMarble(adj) && IsSpace(dest)) {
				Moves[n++].Set(b, dest);
				visited[dest] = true;
			}
		}

		// last loops all the multiple hop jumps
		while (rear != n) {
			const unsigned mid = Moves[rear].dest;
			for (unsigned k = 0; k < HopNbr[mid]; k++) {
				const unsigned adj = HopAdj[mid][k];
				const unsigned dest = Hop[mid][k];
				if (IsMarble(adj) && IsSpace(dest) && !visited[dest]) {
					Moves[n++].Set(b, dest);
					visited[dest] = true;
				}
			}
			rear++;
		}
	}
	// counting sort the index
	unsigned count[33] = {};
	unsigned* const count2 = count + 16;
	for (unsigned i = 0; i < n; i++)
		count2[Moves[i].rank]++;
	std::partial_sum(count, count + 33, count);
	for (unsigned i = n-1; i < n; i--)
			Order[--count2[Moves[i].rank]] = i;
	if (Turn) // default ascending, so reverse the moves if white is playing
		std::reverse(Order, Order + n);
    return n;
}
