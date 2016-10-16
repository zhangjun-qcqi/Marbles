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
	bool Turn; // in the name of white

	unsigned Cord[20]; // cordinates for the marbles; first black then white
	unsigned Invert[81]; // inverted index for cordinates
	int rank;

	constexpr static char marble[2] = { 'b','w' }; // marble[true] = 'w'

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
	int sign() const {return Turn * 2 - 1;} // Let's see what will M$VC do
	bool Quest(const char t,const char b[]);
    void Print(); 
    void MakeMove(const move& m);
	void UndoMove(const move& m);
	unsigned ListMoves(move Moves[], const bool quiescent =false);
};

// set current node using inputs
void node::Set(char t,const char b[])
{
	Turn = t == 'w';
	std::copy(b, b + 81, Board);
	unsigned white = 10;
	unsigned black = 0;
	for (unsigned b = 0; b<81; b++) {
		if (Board[b] == 'b') {
			Invert[b] = black;
			Cord[black++] = b;
		}
		else if (Board[b] == 'w') {
			Invert[b] = white;
			Cord[white++] = b;
		}
	}
	rank = 0;
	for (unsigned b : Cord)
		rank += Rank[b];
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

// apply the move on current node
void node::MakeMove(const move& m)
{
    Board[m.orig]=' ';
    Board[m.dest]= marble[Turn];
	Turn = !Turn; // // Let's see what will M$VC do
	rank += m.rank;
	Cord[Invert[m.orig]] = m.dest;
	Invert[m.dest] = Invert[m.orig];
}

// undo the move on current node
void node::UndoMove(const move& m)
{
	Turn = !Turn;
	rank -= m.rank;
	Cord[Invert[m.dest]] = m.orig;
	Invert[m.orig] = Invert[m.dest];
	Board[m.orig] = marble[Turn];
	Board[m.dest] = ' ';
}

// list all possible moves of current node
unsigned node::ListMoves(move Moves[], const bool quiescent)
{
    unsigned n=0;
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
	std::sort(Moves, Moves + n);// ascending
	if (Turn) // so reverse the moves if white is playing
		std::reverse(Moves, Moves + n);
    return n;
}
