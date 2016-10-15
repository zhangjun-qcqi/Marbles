//========================================================================
// node.hpp
// 2012.9.8-2016.10.13
//========================================================================
#ifndef PSTN_HPP
#define PSTN_HPP

#include<cstring>
#include<algorithm>
#include"move.hpp"
#define NOMINMAX
#include <windows.h>

struct node{//positon
    char Board[9][9];
    char Turn;

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
	constexpr static int offsets[6][2] = {
		{ 0,-1 },//left
		{ -1,0 },//up
		{ 1,-1 },//down left
		{ -1,1 },//up right
		{ 1,0 },//down
		{ 0,1 },//right
	};

	void Init(){Set('w',init);}
    void Set(char t,const char b[]);
	char* begin() { return &Board[0][0]; }
	char* end() { return &Board[8][8]; }
	bool operator==(node& b);
	char At(unsigned i, unsigned j);
	bool Quest(char t,const char b[]);
    void Print(); 
	void Count(int& White, int& Black);
    void MakeMove(move& m);
	void UndoMove(move& m);
	unsigned ListMoves(move Moves[]);

};

// set current node using inputs
void node::Set(char t,const char b[])
{
    Turn=t;
	std::copy(b, b + 81, begin());
}

// ==
inline bool node::operator==(node& b)
{
	return std::equal(begin(), end(), b.begin()) && Turn == b.Turn;
}

// get board cell with boarder checking
inline char node::At(unsigned i, unsigned j)
{
	if (i>8 || j>8) // note the unsigned wrap
		return '.'; //illegal position
	else
		return Board[i][j];
}

// is input node legal? Set to it if so
bool node::Quest(char t,const char bd[])
{
    if(t!='b'&&t!='w')
        return false;
    unsigned w = std::count(begin(), end(), 'w');
    unsigned b = std::count(begin(), end(), 'b');
    unsigned s = std::count(begin(), end(), ' ');
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
			switch (Board[i - j][j]) {
			//case 'w': printf("\033[32m"); break;
			case 'w': SetConsoleTextAttribute(hConsole, 10); break;
			//case 'b': printf("\033[31m"); break;
			case 'b': SetConsoleTextAttribute(hConsole, 12); break;
			}
			printf("%d%d  ", i - j, j);
			//printf("\033[37m");
			SetConsoleTextAttribute(hConsole, 15);
		}
        printf("\n");
    }
}

// evaluate current node
void node::Count(int& White, int& Black)
{
	White = 0;
	Black = 0;
	for (unsigned i = 0; i<9; i++) {
		for (unsigned j = 0; j<9; j++) {
			if (Board[i][j] == 'b')
				Black += i+j;
			else if (Board[i][j] == 'w')
				White += (8-i)+(8-j);
		}
	}
}

// apply the move on current node
void node::MakeMove(move& m)
{
    Board[m.iold][m.jold]=' ';
    Board[m.i][m.j]=Turn;
    Turn=(Turn=='w')?'b':'w';
}

// undo the move on current node
void node::UndoMove(move& m)
{
	Turn = (Turn == 'w') ? 'b' : 'w';
	Board[m.iold][m.jold] = Turn;
	Board[m.i][m.j] = ' ';
}

// list all possible moves of current node
unsigned node::ListMoves(move Moves[])
{
    unsigned n=0;
    for(unsigned i=0;i<9;i++){
        for(unsigned j=0;j<9;j++){
            if(Board[i][j]==Turn){
				// first list the adjacent moves
				for (unsigned k = 0; k < 6; k++) {
					const unsigned ides = i + offsets[k][0];
					const unsigned jdes = j + offsets[k][1];
					if (At(ides, jdes) == ' ')
						Moves[n++].Set(i, j, ides, jdes);
				}

				unsigned rear = n; // prepare the queue
				bool visited[9][9] = {};

				// then list the one hop jumps
				for (unsigned k = 0; k < 6; k++) {
					const unsigned ihop = i + offsets[k][0];
					const unsigned jhop = j + offsets[k][1];
					const unsigned ides = ihop + offsets[k][0];
					const unsigned jdes = jhop + offsets[k][1];
					if (At(ihop, jhop) >= 'b' && At(ides, jdes) == ' ') {
						Moves[n++].Set(i, j, ides, jdes);
						visited[ides][jdes] = true;
					}
				}

				// last loops all the multiple hop jumps
				while (rear != n) {
					for (unsigned k = 0; k < 6; k++) {
						const unsigned ihop = Moves[rear].i + offsets[k][0];
						const unsigned jhop = Moves[rear].j + offsets[k][1];
						const unsigned ides = ihop + offsets[k][0];
						const unsigned jdes = jhop + offsets[k][1];
						if (At(ihop, jhop) >= 'b' && At(ides, jdes) == ' ' && !visited[ides][jdes]) {
							Moves[n++].Set(i, j, ides, jdes);
							visited[ides][jdes] = true;
						}
					}
					rear++;
				}
            }
        }
    }
    return n;
}

#endif // NODE_HPP
