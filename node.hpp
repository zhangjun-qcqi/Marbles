//========================================================================
// node.hpp
// 2012.9.8-2016.10.13
//========================================================================
#pragma once

#include<cstdio>
#include<algorithm>
#include"move.hpp"
#define NOMINMAX
#include <windows.h>

struct node{//positon
    char Board[81];
    char Turn;

	unsigned White[10];
	unsigned Black[10];

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
    void Set(const char t,const char b[]);
	bool operator==(const node& b) const {
		return std::equal(Board, Board+81, b.Board) && Turn == b.Turn;
	}
	bool IsSpace(const unsigned b) const {return Board[b] == ' ';}
	bool IsMarble(const unsigned b) const {return Board[b] != ' ';}
	bool node::Win() const{// white wins?
		return Board[0] == 'w' && Board[1] == 'w' && Board[2] == 'w' && Board[3] == 'w'
			&& Board[9] == 'w' && Board[10] == 'w' && Board[11] == 'w'
			&& Board[18] == 'w' && Board[19] == 'w'
			&& Board[27] == 'w';
	}
	bool node::Lose() const{// white loses?
		return Board[80] == 'b' && Board[79] == 'b' && Board[78] == 'b' && Board[77] == 'b'
			&& Board[71] == 'b' && Board[70] == 'b' && Board[69] == 'b'
			&& Board[62] == 'b' && Board[61] == 'b'
			&& Board[53] == 'b';
	}
	int sign() const {return Turn == 'w' ? 1 : -1;}
	bool Quest(const char t,const char b[]);
    void Print(); 
	int rank() const;
    unsigned* MakeMove(const move& m);
	void UndoMove(const move& m, unsigned* const old);
	unsigned ListMoves(move Moves[], const bool quiescent =false);
};

// set current node using inputs
void node::Set(char t,const char b[])
{
    Turn=t;
	std::copy(b, b + 81, Board);
	unsigned white = 0;
	unsigned black = 0;
	for (unsigned b = 0; b<81; b++) {
		if (Board[b] == 'b')
			Black[black++] = b;
		else if (Board[b] == 'w')
			White[white++] = b;
	}
}

// is input node legal? Set to it if so
bool node::Quest(char t,const char bd[])
{
    if(t!='b'&&t!='w')
        return false;
    unsigned w = std::count(Board, Board + 81, 'w');
    unsigned b = std::count(Board, Board + 81, 'b');
    unsigned s = std::count(Board, Board + 81, ' ');
    if(w!=10||b!=10||w+b+s!=81)
        return false;
    Set(t,bd);
    return true;
}

// print current node
void node::Print()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    for(int i=0;i<17;i++){
		for (int j = 0; j < std::max(8 - i,i-8); j++) printf("  ");
		for (int j = std::max(0,i-8); j <= std::min(i,8); j++) {
			switch (Board[(i - j)*9+j]) {
			//case 'w': printf("\033[32m"); break;
			case 'w': SetConsoleTextAttribute(hConsole, 10); break;
			//case 'b': printf("\033[31m"); break;
			case 'b': SetConsoleTextAttribute(hConsole, 12); break;
			}
			printf("%02d  ", (i - j) * 9 + j);
			//printf("\033[37m");
			SetConsoleTextAttribute(hConsole, 15);
		}
        printf("\n");
    }
}

// evaluate current node
int node::rank() const
{
	int rank = 0;
	for (unsigned b : White)
		rank += 16-Rank[b];
	for (unsigned b : Black)
		rank -= Rank[b];
	return rank;
}

// apply the move on current node
unsigned* node::MakeMove(const move& m)
{
    Board[m.orig]=' ';
    Board[m.dest]=Turn;
	unsigned* Marble;
	if (Turn == 'w') {
		Turn = 'b';
		Marble = White;
	}
	else {
		Turn = 'w';
		Marble = Black;
	}
	unsigned* old = std::find(Marble, Marble + 10, m.orig);
	*old = m.dest;
	return old;
}

// undo the move on current node
void node::UndoMove(const move& m, unsigned* const old)
{
	Turn = (Turn == 'w') ? 'b' : 'w';
	*old = m.orig;
	Board[m.orig] = Turn;
	Board[m.dest] = ' ';
}

// list all possible moves of current node
unsigned node::ListMoves(move Moves[], const bool quiescent)
{
    unsigned n=0;
	unsigned* Marble = (Turn == 'w') ? White : Black;
	for(unsigned i=0;i<10;i++){
		unsigned b = Marble[i];
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
    return n;
}
