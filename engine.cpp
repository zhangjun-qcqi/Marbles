//========================================================================
// engine.cpp
// 2012.9.7-2016.10.13
//========================================================================
#include<cstdio>
#include<cstdlib>
#include<ctime>
#include<algorithm>
#include"node.hpp"
#include"move.hpp"

constexpr unsigned QuietDepth = 6;//start quiescent search when depth > this
constexpr unsigned MaxDepth = QuietDepth+3;//max search depth
constexpr unsigned MaxMoves = 250;
constexpr int Win = 140; // 16 + 15 * 2 + 14 * 3 + 13 * 4
node Curr; // current node; use pairing MakeMove and UndoMove to keep track

int CutoffTest(unsigned Depth, move Moves[MaxMoves], unsigned& movesNbr);
int AlphaBeta(node& Node,move& Move);
int NegaMax(unsigned Depth,int alpha,int beta);
void Play();
void Bench();

int main()
{
	Pre();
    //setbuf(stdout, NULL);
    //srand(time(0));

    //Play();
	Bench();
}

void Bench()
{
	constexpr char * hard =
		"bbbb     "
		" bb      "
		"         "
		" bb      "
		"  bbww   "
		"      ww "
		"       w "
		"      ww "
		"     w ww";
	node Node;
	Node.Set('b',hard);
	Node.Print();
	move Move;
	int a = AlphaBeta(Node, Move);
	printf("%d\n", a);
	Move.Print();
}

void Play()
{
    node Node;
	Node.Init();
	Node.Print();
    move Move;
    move Moves[MaxMoves];

	while(true){
        unsigned n=Node.ListMoves(Moves);
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
                if(Node.Quest(buf[0],buf+2)){
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

int CutoffTest(unsigned Depth, move Moves[MaxMoves], unsigned& movesNbr)
{
	if (Curr.Win())
		return Win * Curr.sign();
	if (Curr.Lose())
		return -Win * Curr.sign();
	if (Depth <= QuietDepth) {
		movesNbr = Curr.ListMoves(Moves);// normal search
		return INT_MAX;//INT_MAX means no cut off
	}
	if (Depth < MaxDepth) {
		movesNbr = Curr.ListMoves(Moves, true);// quiescent search
		if(movesNbr != 0) // noisy node
			return INT_MAX;//INT_MAX means no cut off
	}
	return Curr.rank() * Curr.sign();//evaluate quiet node or max depth node
}

int AlphaBeta(node& Node,move& Move)
{
    int alpha = -INT_MAX;
    int beta = INT_MAX;
    Curr = Node;
	constexpr int Depth = 0;

	move Moves[MaxMoves];//possible moves
	unsigned movesNbr;
    int Utility = CutoffTest(Depth, Moves, movesNbr);
    if(Utility!=INT_MAX)
        return Utility;
	std::sort(Moves, Moves + movesNbr);
	if (Curr.Turn == 'b') // reverse the moves if black is playing
		std::reverse(Moves, Moves + movesNbr);
    for(unsigned i=0;i<movesNbr;i++){
		unsigned* const old = Curr.MakeMove(Moves[i]);
        int score = -NegaMax(Depth + 1,-beta,-alpha);//new utility
		Curr.UndoMove(Moves[i], old);
        if(score>alpha){
            alpha=score;
            Move=Moves[i];
        }
		
    }
    return alpha;
}

int NegaMax(unsigned Depth,int alpha,int beta)
{
    if(alpha==Win) return Win;//pre alpha-prune

	move Moves[MaxMoves];//possible moves
	unsigned movesNbr;
	int Utility = CutoffTest(Depth, Moves, movesNbr);
	if (Utility != INT_MAX)
		return Utility;
	std::sort(Moves, Moves + movesNbr);
	if (Curr.Turn == 'b') // reverse the moves if black is playing
		std::reverse(Moves, Moves + movesNbr);
    for(unsigned i=0;i<movesNbr;i++){
		unsigned* const old = Curr.MakeMove(Moves[i]);
        int score = -NegaMax(Depth+1,-beta,-alpha);//new utility
		Curr.UndoMove(Moves[i], old);
        if(score>=beta) return beta;//beta-prune,fail-hard
        if(score>alpha) alpha=score;
	}
    return alpha;
}
