//========================================================================
// engine.cpp
// 2012.9.7-2019.8.26
//========================================================================
#pragma warning(disable:4996) // for stupid M$VC
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
unsigned QuietDepths[] = {2, 4, 6};
constexpr int QuietLevels = sizeof(QuietDepths) / sizeof(QuietDepths[0]);
int QuietCache[17];
constexpr int Win = 60; // 8 + 7 * 2 + 6 * 3 + 5 * 4
position Curr; // current position
std::unordered_map<hash, transposition> TTable; // transposition table
unsigned Usage;
unsigned Ply;
unsigned Rehash;

int CutoffTest(unsigned Depth, move Moves[MaxBreadth], unsigned& MovesNo);
int AlphaBeta(position& Node,move& Move);
int NegaMax(unsigned Depth, int alpha, int beta, move& Move);
void Play();
void Bench(const char* board, char player, const unsigned depths[],
	const unsigned quiets[]);
void PrepareQuietCache();

int main(int argc, char* argv[])
{
	PreCompute();
	//setbuf(stdout, NULL);

    if (argc <= 1) {
        Play();
    }
    else {
        //Bench(easy, 'b', easyDepths, easyQuiets);
        if (strcmp(argv[1], "medium") == 0) {
            Bench(medium, 'b', mediumDepths, mediumQuiets);
        }
    }

}

void PrepareQuietCache()
{
    constexpr int Bars[QuietLevels + 1] = {-16, 1, 2, 4};
	for (unsigned d = 0; d < MaxDepth; d++) {
		QuietCache[d] = Bars[std::lower_bound(QuietDepths,
			QuietDepths + QuietLevels, d) - QuietDepths];
	}
}

void Bench(const char * board, char player, const unsigned depths[],
	const unsigned quiets[])
{
	MaxDepth = depths[0];
	LeafDepth = depths[1];
	std::copy(quiets, quiets + QuietLevels, QuietDepths);
	PrepareQuietCache();

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
	printf("%d / %zu = %f\n", Usage, TTable.size(),
		float(Usage) / TTable.size());
	printf("load factor = %f\n", TTable.load_factor());
	printf("rehash = %d\n", Rehash);
}

void Play()
{
	PrepareQuietCache();

	position Node;
	Node.Init();
	Node.Print();
	move Move;
	move Moves[MaxBreadth];

	while(true){
		unsigned n = Node.ListMoves(Moves);
		char buf[128];
		int ch;
		while(fgets(buf,sizeof(buf),stdin)!=0){
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
			else if(len==84 && (buf[1]==':' || buf[1]=='|')) {//Board
				if(Node.IsLegal(buf[0], buf + 2)){
					Node.Set(buf[0], buf + 2);
                    Node.Print();
                    if (buf[1] == ':') {
                        n = Node.ListMoves(Moves);
                        continue;
                    }
                    else
                        break;
				}
			}
		}
		printf("thinking...\n");
		auto tstart = std::chrono::high_resolution_clock::now();
		if (Node.Score[0] == -Win) {
			printf("black wins\n");
			break;
		}
		if (Node.Score[1] == Win) {
			printf("white wins\n");
			break;
		}
		int Utility = AlphaBeta(Node,Move);
		auto tend = std::chrono::high_resolution_clock::now();
		printf("time = %lld ms\n", std::chrono::duration_cast<
			std::chrono::milliseconds>(tend - tstart).count());
		printf("score = %d\n", Utility);
		auto OldSize = TTable.size();
		printf("%d / %zu = %f\n", Usage, OldSize, (float)Usage / OldSize);
		Move.Print();
		Node.MakeMove(Move);
		Node.Print();
		for (auto it = TTable.begin(); it != TTable.end();) {
			if (it->second.Age + 3 < Ply)
				it = TTable.erase(it);
			else
				++it;
		}
		auto NewSize = TTable.size();
		printf("ply=%u, %zu erased, %zu remaining\n", Ply,
			OldSize - NewSize, NewSize);
		Usage = 0;
		Ply++;
	}
}

int CutoffTest(unsigned Depth, move Moves[MaxBreadth], unsigned& MovesNo)
{
	if (Depth < MaxDepth && Curr.Score[0] != -Win && Curr.Score[1] != Win) {
		// normal or quiescent search
		MovesNo = Curr.ListMoves(Moves, QuietCache[Depth]);
		if(MovesNo != 0) // noisy position
			return -NoCutOff;
	}
	//evaluate quiet position or max depth position or terminal position
	const int s = Curr.Score[0] + Curr.Score[1];
	if (Curr.WhiteTurn)
		return s;
	else
		return -s;
}

int AlphaBeta(position& Node,move& Move)
{
	Curr = Node;
	return NegaMax(0, -NoCutOff, NoCutOff, Move);
}

#ifndef NDEBUG
constexpr unsigned long long ulls[] = {0x0, 0x0, 0x0};
const hash wow = ulls2hash(ulls);
#endif

int NegaMax(unsigned Depth, int alpha, int beta, move& Move)
{
	int alphaOrig = alpha;
	if(alpha==Win) return Win;//pre alpha-prune

#ifndef NDEBUG
	if (Curr.Hash == wow)
		Curr.Print();
#endif
	transposition oldT;
	bool hasOldT = false;
	if(Depth < LeafDepth // ignore leaves
		&& TTable.count(Curr.Hash) != 0){
		oldT = TTable[Curr.Hash];
		hasOldT = true;
		TTable[Curr.Hash].Age = Ply; // update age when access
		if(oldT.Depth <= Depth){
			Usage++;
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
				printf("pick %zu from %u\n", i, MovesNo);
			}
		}
	}
	for(unsigned i=0;i<MovesNo;i++){
		const move m = Moves[i];
		Curr.MakeMove(m);
		move _;
		int score = -NegaMax(Depth+1, -beta, -alpha, _);
		Curr.UndoMove(m);
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
	if (Curr.Hash == wow)
		Curr.Print();
#endif
	if (Depth < LeafDepth){ // ignore leaves
		transposition newT = { best, best, Depth, Move, Ply };
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
				oldT.Print();
				printf(" > ");
				newT.Print();
				printf("\n");
				TTable[Curr.Hash].Age = Ply;
			}
		}
		else {
			const auto OldBucket = TTable.bucket_count();
			TTable[Curr.Hash] = newT;
			const auto NewBucket = TTable.bucket_count();
			if (OldBucket != NewBucket) {
				Rehash++;
				printf("rehash %u: %zu > %zu\n", Rehash, OldBucket, NewBucket);
			}
		}
	}
	return best;
}
