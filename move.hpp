//========================================================================
// move.hpp
// 2012.9.8-2016.10.13
//========================================================================
#ifndef MOVE_HPP
#define MOVE_HPP

#include<cstring>
#include<cstdio>

struct move{
    unsigned iold,jold,i,j;

    void Set(unsigned row,unsigned col, unsigned row2, unsigned col2);
	void Set(const char b[]) {
		Set(b[0] - '0', b[1] - '0', b[2] - '0', b[3] - '0');
	}
    void Print();
	bool operator==(const move& b) {
		return i == b.i && j == b.j && iold == b.iold && jold == b.jold;
	}
	bool operator<(const move& b) {
		return i + b.iold + j + b.jold < b.i + iold + b.j + jold;
	}
};

inline void move::Set(unsigned row,unsigned col, unsigned row2, unsigned col2)
{
	iold = row;
	jold = col;
	i = row2;
	j = col2;
}

void move::Print()
{
    printf("%d%d%d%d\n", iold, jold, i, j);
}

#endif // MOVE_HPP
