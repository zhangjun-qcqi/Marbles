//========================================================================
// engine.cpp
// 2012.9.7-2019.8.29
//========================================================================
#ifdef _MSC_VER
    #pragma warning(disable:4996) // for stupid M$VC
#endif
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <array>
#include <unordered_map>
#include "position.hpp"
#include "move.hpp"
#include "transposition.hpp"
#include "config.hpp"

unsigned MaxDepth = 11; // max search depth
unsigned LeafDepth = MaxDepth - 2; // ignore deeper transpositions
std::array<unsigned, 3> QuietDepths = {2, 4, 6};
int BarCache[17]; // the bar of moves for each depth
constexpr int Win = 60; // 8 + 7 * 2 + 6 * 3 + 5 * 4
position Curr; // current position
std::unordered_map<hash, transposition> TTable; // transposition table
std::array<unsigned, 9> Usage; // how many times TTable has been used (by depth)
unsigned Ply; // used to track ply for TTable's entry
unsigned Rehash; // how many times TTable has been rehashed

int CutoffTest(unsigned Depth, move Moves[MaxBreadth], unsigned& MovesNo);
int AlphaBeta(position& Node,move& Move);
int NegaMax(unsigned Depth, int alpha, int beta, move& Move);
void Play();
void Bench(config Config);
void PrepareQuietCache();

int main(int argc, char* argv[])
{
	PreCompute();
	//setbuf(stdout, NULL);

    if (argc <= 1) {
        Play();
    }
    else {
		if (strcmp(argv[1], "easy") == 0) {
			Bench(Easy);
		}
        else if (strcmp(argv[1], "medium") == 0) {
            Bench(Medium);
        }
    }
}

// for each position, listed moves will have scores like
// 1, 2, 4, 6, 8, 10, 12, 14, 16,
//-1,-2,-4,-6,-8,-10,-12,-14,-16, and 0
// not all moves need to be listed for analyzing
// moves are categorized into 4 groups
// score <=0; bad move, reasonable player won't do this
// score = 1; humble move, useful for building ladders
// score = 2; normal move, a plain hop
// score >= 4; good move, chain-hopping, most prefered
// the idea is the top nodes will search all the moves
// while the nodes more near the leaves will search lesser
void PrepareQuietCache()
{
    constexpr int Bars[QuietDepths.size() + 1] = {-16, 1, 2, 4};
	for (unsigned d = 0; d < MaxDepth; d++) {
        // lower_bound returns an iterator pointing to the first element
        // in the range that is not less than the value
		BarCache[d] = Bars[std::lower_bound(QuietDepths.cbegin(),
			QuietDepths.cend(), d) - QuietDepths.cbegin()];
	}
}

void Bench(config Config)
{
	MaxDepth = Config.MaxDepth;
	LeafDepth = Config.TTableDepth;
    QuietDepths = Config.QuietDepths;
	PrepareQuietCache();

	position Node;
	Node.Set(Config.Player, Config.Board);
	Node.Print();
	move Move;
	auto tstart = std::chrono::high_resolution_clock::now();
	int Score = AlphaBeta(Node, Move);
	auto tend = std::chrono::high_resolution_clock::now();
	printf("time = %lld ms\n", (long long) std::chrono::duration_cast<
		std::chrono::milliseconds>(tend - tstart).count());
	printf("score = %d\n", Score);
	Move.Print();
    int SumUsage = std::accumulate(Usage.cbegin(), Usage.cend(), 0);
	printf("%d / %zu = %f\n", SumUsage, TTable.size(),
		float(SumUsage) / TTable.size());
    for (auto u: Usage)
        printf("%d ", u);
	printf("\nload factor = %f\n", TTable.load_factor());
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
                    TTable.clear();
                    Ply = 0;
                    Usage = {};
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
		printf("time = %lld ms\n", (long long) std::chrono::duration_cast<
			std::chrono::milliseconds>(tend - tstart).count());
		printf("score = %d\n", Utility);
		auto OldSize = TTable.size();
        int SumUsage = std::accumulate(Usage.cbegin(), Usage.cend(), 0);
		printf("%d / %zu = %f\n", SumUsage, OldSize, (float)SumUsage / OldSize);
        for (auto u: Usage)
            printf("%d ", u);
        printf("\n");
		Move.Print();
		Node.MakeMove(Move);
		Node.Print();
		for (auto it = TTable.begin(); it != TTable.end();) {
            // statistics shows that entry older than 3 plies are seldom used
			if (it->second.Ply + 3 < Ply)
				it = TTable.erase(it);
			else
				++it;
		}
		auto NewSize = TTable.size();
		printf("ply=%u, %zu erased, %zu remaining\n", Ply,
			OldSize - NewSize, NewSize);
		Usage = {};
		Ply++;
	}
}

int CutoffTest(unsigned Depth, move Moves[MaxBreadth], unsigned& MovesNo)
{
	if (Depth < MaxDepth && Curr.Score[0] != -Win && Curr.Score[1] != Win) {
		// normal or quiescence search
		MovesNo = Curr.ListMoves(Moves, BarCache[Depth]);
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
		TTable[Curr.Hash].Ply = Ply; // update ply when access
		if(oldT.Depth <= Depth){
			Usage[Depth]++;
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
				TTable[Curr.Hash].Ply = Ply;
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
