#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <stack> 
#include <cmath>
#include <tuple>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <chrono> 
#include <random>

using namespace std;

enum ValueType { EXACT, LOWERBOUND, UPPERBOUND };
// the entries to store in TT;
typedef struct TTEntry {
	unsigned long long int hash;			// 64+ bits
	unsigned long long int best_move_hash;	// hash after best move
	string bestMove;						// node best move
	int depth = -1;							// node depth
	int value;								// node value
	ValueType f;							// bound type
}TTEntry;

class Board {
	static int n; //dimention
	static char color;
	static string move;
	static bool game_over;
	static unsigned int fb;
	static unsigned int fw;
	static unordered_multiset<string>Mb;
	static unordered_multiset<string>Mw;
	static string CMD;
	static string JumpsAllowed;

	//...........................Hash info & TT...............................
	static unsigned long long int ZobristTable[8][8][2];// hash value table
	static unsigned long long int current_hash;			// current state hash
	static unsigned long int function_calls;
	static unordered_map<string, unsigned long long int> HH; //to save moves for sorting
	static vector<TTEntry> TT;// transposition table;
	static int depth;
	static int search_mode;
	static int current_turn;
	//....................................timing...........................
	static double search_time_ft;
	static double search_time_rt;
	static double total_time;
	static double extra_time;//time remaining after game ends

public:
	static array<string, 64> board;
	static stack<array<string, 64>> history;
	
	//.........................................................................
public:
	void setSize(string input) const;
	void setCMD(string cmd);
	void setColor(char c);
	void setDepth(int _depth);
	void setMode(int mode);
	void setTime(const double& time)const;
	void setMove(string& m);

	void switchPlayer();
	//the default simple ATAXX board
	void DisplayBoard();
	//get board from user
	void getBoard(array<string, 64>& board);
	//the board the user choses to set with command 's'
	void PrintBoard();
	//parse the move
	tuple <int, int, int, int> parseMove(string move);
	//undo move
	void undoMove(stack<array<string, 64>>& history, array<string, 64>& board);
	//check legal moves & make sure its in wanted format && within range(a~h) & (1-8)
	bool isLegalMove(int x, int y, int xx, int yy);
	//check if te move is clone 
	bool isCloneMove(int x1, int y1, int x2, int y2);
	//make movements (jump/ clone/ pass)
	void makeMove(bool is_Clone, int x1, int y1, int x2, int y2,array<string, 64>& board);
	//check the score for any player w or b
	int getScore(char color);
	//return value in view of player to move
	int getValue();
	//get all possible legal moves for chosen player
	vector<tuple<int, int, int, int>> getLegalMoves(char c);
	//check if poth players has no valid legal moves anymore
	bool nolegalMovesLeft();
	//get children of current node (move,move order)
	vector<pair<string, unsigned long long int>> getChildren(char color);

	//helper functions
	int findCol(char p);
	string findRow(int x);
	bool FindNeighbors(int x, int y, int to_x, int to_y);
	string ReconstructMove(int x1, int y1, int x2, int y2); //get move string from indices
	//game functions
	void play(unsigned int& fb, unsigned int& fw, unordered_multiset<string>& Mb, unordered_multiset<string>& Mw, char color, string move, array<string, 64>& board, string& JumpsAllowed, stack<array<string, 64>>& history);
	bool gameOver();
	int RepetitionCount(string move, unordered_multiset<string> SavedMoves);//check for 3 move repetition rule
	static bool SortChild(const pair<string, unsigned long long int> &a, const pair<string, unsigned long long int> &b);
	//-------------------Hash T---------------------------------------
	unsigned long long int getHash()const;
	unsigned long long int randomInt();
	void initTable(int n);
	unsigned long long int computeHash(array<string, 64>& board);
	int indexOf(char color);
	TTEntry& checkTT(const unsigned long long int& _hash, int depth, int& alpha, int& beta)const;
	void StoreEntry(TTEntry& e, unsigned long long& _hash, int& score, int& depth, int& alpha, int& beta, string& _bestMove, unsigned long long int& best_move_hash) const;

	//------------------------search--------------------------------------
	int AlphaBeta(int depth,char color, array<string, 64>& board, int alpha, int beta, stack<array<string, 64>>& history);
	int NegaScout(int depth, char color, array<string, 64>& board, int alpha, int beta, stack<array<string, 64>>& history);
	//-------------------------------GAME---------------------------------
	void Game();
	};
