//========================================================================
// engine.cpp
// 2012.9.7-2018.4.11
//========================================================================
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <string>
#include "position.hpp"
#include "move.hpp"
#include "transposition.hpp"

constexpr unsigned QuietDepth = 6;//start quiescent search when depth > this
constexpr unsigned MaxDepth = QuietDepth+3;//max search depth
constexpr int Win = 60; // 8 + 7 * 2 + 6 * 3 + 5 * 4
constexpr int NoCutOff = 137; // also represents an impossible score
position Curr; // current position
std::unordered_map<hash, transposition> TTable; // transposition table
int collision;

int CutoffTest(unsigned Depth, move Moves[MaxBreadth],
	unsigned Index[MaxBreadth], unsigned& MovesNo);
int AlphaBeta(position& Node,move& Move);
int NegaMax(unsigned Depth, int alpha, int beta, move& Move);
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
	printf("%d / %llu = %f\n", collision, TTable.size(), collision * 1.0 / TTable.size());
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
	const int sign = Curr.WhiteTurn ? 1 : -1;
	if (Curr.Score[0] == -Win)
		return 2 * -Win * sign;
	if (Curr.Score[1] == Win)
		return 2 * Win * sign;
	if (Depth <= QuietDepth) {
		MovesNo = Curr.ListMoves(Moves, Index);// normal search
		return -NoCutOff;
	}
	if (Depth < MaxDepth) {
		MovesNo = Curr.ListMoves(Moves, Index, true);// quiescent search
		if(MovesNo != 0) // noisy position
			return -NoCutOff;
	}
	//evaluate quiet position or max depth position
	return (Curr.Score[0] + Curr.Score[1]) * sign;
}

int AlphaBeta(position& Node,move& Move)
{
	Curr = Node;
	return NegaMax(0, -NoCutOff, NoCutOff, Move);
}

int NegaMax(unsigned Depth, int alpha, int beta, move& Move)
{
	static int call = 0;
	call++;
	int alphaOrig = alpha;
	if(alpha==Win) return Win;//pre alpha-prune

	//const hash wow("00000011010001000011010001100001000010000000011101000000000000000101010100000000000000100000000000000001000000000101000001010001");
	//if (Curr.Hash == wow) {
	//	Curr.Print();
	//}
	bool isCorner = Curr.Hash[126];
	transposition oldT;
	bool hasOldT = false;
	if(!isCorner && TTable.count(Curr.Hash) != 0){
		oldT = TTable[Curr.Hash];
		hasOldT = true;
		if(oldT.Depth <= Depth){
			Move = oldT.Move;
			if(oldT.ScoreType == scoretype::exact)
				return oldT.Score;
			else if (oldT.ScoreType == scoretype::lowerbound)
				alpha = std::max(alpha, oldT.Score);
			else
				beta = std::min(beta, oldT.Score);
			if (alpha >= beta)
				return oldT.Score;
		}
	}

	move Moves[MaxBreadth];//possible moves
	unsigned Index[MaxBreadth];
	unsigned MovesNo;
	int best = CutoffTest(Depth, Moves, Index, MovesNo);
	if (best != -NoCutOff)
		return best; // terminal node does not need a move
	for(unsigned i=0;i<MovesNo;i++){
		const move m = Moves[Index[i]];
		//auto Old = Curr;
		Curr.MakeMove(m);
		move dummy;
		int score = -NegaMax(Depth+1, -beta, -alpha, dummy);
		Curr.UndoMove(m);
		//if (!std::equal((char*)&Curr, (char*)&Curr + sizeof(Curr), (char*)&Old)) {
		//	Curr.Print();
		//	Old.Print();
		//}
		if (score > best) {
			best = score;
			Move = m;
			if (score > alpha) {
				alpha = score;
				if (score >= beta) break;//beta-prune, fail-soft
			}
		}
	}

	if (!isCorner && Depth <= QuietDepth){ // only store in table the nodes near the root
		transposition newT;
		newT.Score = best;
		newT.Depth = Depth;
		newT.Move = Move;
		if(best <= alphaOrig)
			newT.ScoreType = scoretype::upperbound;
		else if (best >= beta)
			newT.ScoreType = scoretype::lowerbound;
		else
			newT.ScoreType = scoretype::exact;

		//if (Curr.Hash == wow) {
		//	Curr.Print();
		//}
		if(hasOldT){
			//auto oldT = TTable[Curr.Hash];
			//std::string s = Curr.Hash.to_string();
			//printf("%llu\n", Curr.Hash);
			if (oldT.ScoreType == scoretype::exact) {
				Curr.Print();
				collision++;
			}
			else if (newT.ScoreType != scoretype::exact
				&& oldT.ScoreType != newT.ScoreType) {
				if (oldT.Score != newT.Score) {
					Curr.Print();
					collision++;
				}
				else {
					newT.ScoreType = scoretype::exact;
				}
			}
		}
		TTable[Curr.Hash] = newT;
	}
	return best;
}
