//========================================================================
// engine.cpp
// 2012.9.7-2018.4.17
//========================================================================
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include "position.hpp"
#include "move.hpp"
#include "transposition.hpp"

unsigned MaxDepth = 11; // max search depth
unsigned LeafDepth = MaxDepth - 2; // ignore deeper transpositions
unsigned QuietDepths[] = {3, 5, 7};
constexpr int Win = 60; // 8 + 7 * 2 + 6 * 3 + 5 * 4
constexpr int NoCutOff = 137; // also represents an impossible score
position Curr; // current position
std::unordered_map<hash, transposition> TTable; // transposition table
int usage;

int CutoffTest(unsigned Depth, move Moves[MaxBreadth], unsigned& MovesNo);
int AlphaBeta(position& Node,move& Move);
int NegaMax(unsigned Depth, int alpha, int beta, move& Move);
void Play();
void Bench(const char * board, char player, const unsigned depths[],
	const unsigned quiets[]);

int main()
{
	PreCompute();
	//setbuf(stdout, NULL);

	//Play();
	Bench(easy, 'b', easyDepths, easyQuiets);
	//Bench(medium, 'b', mediumDepths, mediumQuiets);
}

void Bench(const char * board, char player, const unsigned depths[],
	const unsigned quiets[])
{
	MaxDepth = depths[0];
	LeafDepth = depths[1];
	std::copy(quiets, quiets + 3, QuietDepths);
	position Node;
	Node.Set('b', board);
	Node.Print();
	move Move;
	auto tstart = std::chrono::high_resolution_clock::now();
	int a = AlphaBeta(Node, Move);
	auto tend = std::chrono::high_resolution_clock::now();
	printf("time = %lld ms\n", std::chrono::duration_cast<
		std::chrono::milliseconds>(tend - tstart).count());
	printf("score = %d\n", a);
	Move.Print();
	printf("%d / %zu = %f\n", usage, TTable.size(),
		float(usage) / TTable.size());
}

void Play()
{
	position Node;
	Node.Init();
	Node.Print();
	move Move;
	move Moves[MaxBreadth];

	while(true){
		unsigned n = Node.ListMoves(Moves);
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
		auto tstart = std::chrono::high_resolution_clock::now();
		int Utility = AlphaBeta(Node,Move);
		auto tend = std::chrono::high_resolution_clock::now();
		printf("time = %lld ms\n", std::chrono::duration_cast<
			std::chrono::milliseconds>(tend - tstart).count());
		printf("score = %d\n", Utility);
		printf("%d / %zu = %f\n", usage, TTable.size(),
			usage * 1.0 / TTable.size());
		Move.Print();
		Node.MakeMove(Move);
		Node.Print();
		//TTable.clear();
		usage = 0;
	}
}

int CutoffTest(unsigned Depth, move Moves[MaxBreadth], unsigned& MovesNo)
{
	const int sign = Curr.WhiteTurn ? 1 : -1;
	if (Curr.Score[0] == -Win)
		return 2 * -Win * sign;
	if (Curr.Score[1] == Win)
		return 2 * Win * sign;
	unsigned bar = unsigned(std::lower_bound(QuietDepths, QuietDepths + 3,
		Depth) - QuietDepths);
	if (Depth < MaxDepth) {
		MovesNo = Curr.ListMoves(Moves, bar);// normal or quiescent search
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

#ifndef NDEBUG
constexpr unsigned long long ulls[] = {0x0, 0x0};
const hash wow = ulls2hash(ulls);
#endif

int NegaMax(unsigned Depth, int alpha, int beta, move& Move)
{
	int alphaOrig = alpha;
	if(alpha==Win) return Win;//pre alpha-prune

#ifndef NDEBUG
	if (Curr.Hash == wow) {
		Curr.Print();
	}
#endif
	bool isCorner = Curr.Hash[126]; // we don't use corner transpositions
	transposition oldT;
	bool hasOldT = false;
	if(!isCorner && Depth < LeafDepth // ignore corners and leaves
		&& TTable.count(Curr.Hash) != 0){
		oldT = TTable[Curr.Hash];
		hasOldT = true;
		if(oldT.Depth <= Depth){
			usage++;
			Move = oldT.Move;
			if(oldT.Lowerbound == oldT.Upperbound)
				return oldT.Lowerbound;
			if (oldT.Lowerbound >= beta)
				return oldT.Lowerbound;
			if (alpha >= oldT.Upperbound)
				return oldT.Upperbound;
			alpha = std::max(alpha, oldT.Lowerbound);
			beta = std::min(beta, oldT.Upperbound);
		}
	}

	move Moves[MaxBreadth];//possible moves
	unsigned MovesNo;
	int best = CutoffTest(Depth, Moves, MovesNo);
	if (best != -NoCutOff)
		return best; // terminal node does not need a move
	if (hasOldT && oldT.Lowerbound != -NoCutOff && oldT.Depth <= Depth) {
		if (oldT.Move != Moves[0]) { // move ordering
			size_t i = std::find(Moves+1, Moves+MovesNo,oldT.Move)-Moves;
			if (i != MovesNo) {
				std::swap(Moves[0], Moves[i]);
				printf("%zu is old best from [0 %u)\n", i, MovesNo);
			}
		}
	}
	for(unsigned i=0;i<MovesNo;i++){
		const move m = Moves[i];
#ifdef DEBUG_MAKE_MOVE
		auto Old = Curr;
#endif
		Curr.MakeMove(m);
		move _;
		int score = -NegaMax(Depth+1, -beta, -alpha, _);
		Curr.UndoMove(m);
#ifdef DEBUG_MAKE_MOVE
		if (Old != Curr) {
			Curr.Print();
			Old.Print();
		}
#endif
		if (score > best) {
			best = score;
			Move = m;
			if (score > alpha) {
				alpha = score;
				if (score >= beta) break;//beta-prune, fail-soft
			}
		}
	}

#ifndef NDEBUG
	if (Curr.Hash == wow) {
		Curr.Print();
	}
#endif
	if (!isCorner && Depth < LeafDepth){ // ignore corners and leaves
		transposition newT = { best, best, Depth, Move };
		if (best <= alphaOrig) {
			newT.Lowerbound = -NoCutOff;
			newT.Move = NullMove;
		}
		else if (best >= beta)
			newT.Upperbound = NoCutOff;

		if(hasOldT){
			if (Depth == oldT.Depth) {
				if (newT.Move == NullMove)
					newT.Move = oldT.Move;
				newT.Lowerbound = std::max(oldT.Lowerbound, newT.Lowerbound);
				newT.Upperbound = std::min(oldT.Upperbound, newT.Upperbound);
				TTable[Curr.Hash] = newT;
			}
			else if (Depth < oldT.Depth) {
				TTable[Curr.Hash] = newT;
			}
			else { // shallower score vs deeper score, which is better?
				//Curr.Print();
				printf("old is shallower [%d %d]@%u vs [%d %d]@%u\n",
					oldT.Lowerbound, oldT.Upperbound, oldT.Depth,
					newT.Lowerbound, newT.Upperbound, newT.Depth);
			}
		}
		else
			TTable[Curr.Hash] = newT;
	}
	return best;
}
