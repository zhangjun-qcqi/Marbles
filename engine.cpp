//========================================================================
// engine.cpp
// 2012.9.7-2018.3.30
//========================================================================
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include "position.hpp"
#include "move.hpp"

constexpr unsigned QuietDepth = 6;//start quiescent search when depth > this
constexpr unsigned MaxDepth = QuietDepth+3;//max search depth
constexpr int Win = 60; // 8 + 7 * 2 + 6 * 3 + 5 * 4
constexpr int NoCutOff = 137; // also represents an impossible score
position Curr; // current position

int CutoffTest(unsigned Depth, move Moves[MaxBreadth],
	unsigned Index[MaxBreadth], unsigned& MovesNo);
int AlphaBeta(position& Node,move& Move);
int NegaMax(unsigned Depth,int alpha,int beta);
void Play();
void Bench();

int main()
{
	PreCompute();
	//setbuf(stdout, NULL);

	//Play();
	Bench();
}

void Bench()
{
	const char* hard =
		"bbbb     "
		" bb      "
		"         "
		" bb      "
		"  bbww   "
		"      ww "
		"       w "
		"      ww "
		"     w ww";
	position Node;
	Node.Set('b',hard);
	Node.Print();
	move Move;
	auto tstart = std::chrono::high_resolution_clock::now();
	int a = AlphaBeta(Node, Move);
	auto tend = std::chrono::high_resolution_clock::now();
	printf("time = %lld ms\n", std::chrono::duration_cast<
		std::chrono::milliseconds>(tend - tstart).count());
	printf("score = %d\n", a);
	Move.Print();
}

void Play()
{
	position Node;
	Node.Init();
	Node.Print();
	move Move;
	move Moves[MaxBreadth];
	unsigned Index[MaxBreadth];

	while(true){
		unsigned n = Node.ListMoves(Moves, Index);
		char buf[20];
		int ch;
		while(fgets(buf,20,stdin)!=0){
			size_t len = strlen(buf);
			if(buf[len-1]!='\n'){//current line not read all, dicard them
				while ((ch = getc(stdin)) != EOF && ch != '\n');
				continue;
			}
			else if(len==5){
				if(strcmp(buf,"quit\n")==0)//quit
					return;
				if(strcmp(buf,"init\n")==0)//AI plays first
					break;
				Move.Set(buf);
				if(Moves+n!=std::find(Moves,Moves+n,Move)){
					Node.MakeMove(Move);
					break;
				}
			}
			else if(len==19 && buf[1]==':'){//Board
				if(Node.IsLegal(buf[0], buf + 2)){
					Node.Set(buf[0], buf + 2);
					break;
				}
			}
		}
		printf("thinking...\n");
		int Utility = AlphaBeta(Node,Move);
		printf("%d\n", Utility);
		Move.Print();
		Node.MakeMove(Move);
		Node.Print();
	}
}

int CutoffTest(unsigned Depth, move Moves[MaxBreadth],
	unsigned Index[MaxBreadth], unsigned& MovesNo)
{
	if (Curr.Score[0] == -Win)
		return 2 * -Win * Curr.Turn;
	if (Curr.Score[1] == Win)
		return 2 * Win * Curr.Turn;
	if (Depth <= QuietDepth) {
		MovesNo = Curr.ListMoves(Moves, Index);// normal search
		return NoCutOff;
	}
	if (Depth < MaxDepth) {
		MovesNo = Curr.ListMoves(Moves, Index, true);// quiescent search
		if(MovesNo != 0) // noisy position
			return NoCutOff;
	}
	//evaluate quiet position or max depth position
	return (Curr.Score[0] + Curr.Score[1]) * Curr.Turn;
}

int AlphaBeta(position& Node,move& Move)
{
	int alpha = -NoCutOff;
	int beta = NoCutOff;
	Curr = Node;
	constexpr int Depth = 0;

	move Moves[MaxBreadth];//possible moves
	unsigned Index[MaxBreadth];
	unsigned MovesNo;
	int Utility = CutoffTest(Depth, Moves, Index, MovesNo);
	if(Utility != NoCutOff)
		return Utility;
	for(unsigned i=0;i<MovesNo;i++){
		Curr.MakeMove(Moves[Index[i]]);
		int score = -NegaMax(Depth + 1,-beta,-alpha);//new utility
		Curr.UndoMove(Moves[Index[i]]);
		if(score>alpha){
			alpha=score;
			Move=Moves[Index[i]];
		}
	}
	return alpha;
}

int NegaMax(unsigned Depth,int alpha,int beta)
{
	if(alpha==Win) return Win;//pre alpha-prune

	move Moves[MaxBreadth];//possible moves
	unsigned Index[MaxBreadth];
	unsigned MovesNo;
	int Utility = CutoffTest(Depth, Moves, Index, MovesNo);
	if (Utility != NoCutOff)
		return Utility;
	for(unsigned i=0;i<MovesNo;i++){
		Curr.MakeMove(Moves[Index[i]]);
		int score = -NegaMax(Depth+1,-beta,-alpha);//new utility
		Curr.UndoMove(Moves[Index[i]]);
		if(score>=beta) return beta;//beta-prune,fail-hard
		if(score>alpha) alpha=score;
	}
	return alpha;
}
