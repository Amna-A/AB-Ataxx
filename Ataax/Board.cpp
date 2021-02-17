#include "Board.h"

int Board::n; //dimention
array<string, 64> Board::board;
stack<array<string, 64>> Board::history;

char Board::color;
string Board::move;
string Board::best_move = "ms1s2";
unsigned int Board::fb = 0;
unsigned int Board::fw = 0;
unordered_multiset<string> Board::Mb;
unordered_multiset<string> Board::Mw;
string Board::CMD;
bool Board::game_over = false;
string Board::JumpsAllowed = "";
int Board::depth = 20; //set number for case rt with no depth entered

Report Board::record;

//..............................Search.................................
int Board::search_mode;
int Board::current_turn = 1;

//...........................Timing....................................
double Board::search_time_ft;
double Board::search_time_rt;
double Board::total_time = 0;
double Board::extra_time;//time remaining after game ends
int Board::TimeFlag;

//...........................Hash info...................................
unsigned long long int Board::ZobristTable[8][8][2];// hash value table
unsigned long long int Board::current_hash;			// current state hash
unsigned long int Board::function_calls;
unordered_map<string, unsigned long long int> Board::HH; //to save moves for sorting
vector<TTEntry> Board::TT(256000);
mt19937 mt(01234567);
//---------------------------------functions------------------------------
void Board::setCMD(string cmd) {
	CMD = cmd;
}
void Board::setColor(char c) {
	color = c;
}
void Board::setDepth(int _depth) {
	depth = _depth;
}
void Board::switchPlayer() {
	char oponent_color = (color == 'b') ? 'w' : 'b';
	setColor(oponent_color);
}
void Board::setSize(string input) const {
	n = input[2] - '0';
}
void Board::setMode(int mode) {
	search_mode = mode;
}
void Board::setTime(const double& time)const {

	if (search_mode == 1) {
		total_time = 0;
		extra_time = min(time * 0.95, 0.5);
		search_time_ft = time - extra_time;
		TimeFlag = 1;
	}
	else if (search_mode == 2) {
		search_time_ft = time;
		search_time_rt = 31622400;
		TimeFlag = 2;
	}
}
void Board::setMove(string& m) {
	move = m;
}
void Board::setBestMove(string& m) {
	best_move = m;
}

void Board::DisplayBoard() {
	//early made for testing and displaying board upon initialization
	int count = 0;
	// Printing the board headers:
	cout << "     ";
	for (int i = 97; i < 105; i++) { //print a~h
		cout << char(i) << "   ";
		count++;
		if (count == n) {
			break;
		}
	}
	cout << endl;
	// Draw the Board
	vector<vector<char>> temp;
	// create an empty (nxn) 
	temp.resize(n + 1, vector<char>(n + 1, ' '));
	int cnt = temp.size() - 1;
	for (size_t i = 1; i < temp.size(); i++) {
		cout << cnt << " ";
		cnt--;
		for (size_t j = 0; j < temp[i].size() - 1; j++) {
			if ((i == 1 && j == 0) || (i == n && j == n - 1)) {
				cout << "  " << temp[i][j] << "w";
				continue;
			}
			if ((i == 1 && j == n - 1) || (i == n && j == 0)) {
				cout << "  " << temp[i][j] << "b";
				continue;
			}
			cout << "  " << temp[i][j] << "-";
		}
		cout << endl;
	}
}

void Board::getBoard(array<string, 64>& board) {
	//accepting board line by line seperated by Enter key
	cout << "\nEnter your Board: " << endl;
	for (int i = 0; i < n; i++) {
		getline(cin, board[i]);
	}
}
void Board::PrintBoard() {
	int count = 0;
	// Printing the board headers:
	cout << "   ";
	for (int i = 97; i < 105; i++) { //print a~h
		cout << char(i) << "   ";
		count++;
		if (count == n) {
			break;
		}
	}
	cout << endl;
	// Draw the Board
	int r = n;
	for (auto& row : board) {
		cout << r << "  ";
		r--;
		for (auto& s : row) {
			if (row.empty()) { continue; }
			if (s == 'e') { s = '-'; }
			cout << s << "   ";
		}
		if (r < 1) { break; }
		cout << endl;
	}
}

tuple<int, int, int, int> Board::parseMove(string move) {
	int from_x, from_y, to_x, to_y;
	
	from_x = n - (move[2] - '0'); //the row in board, x1
	from_y = findCol(move[1]); //the col in board, p1
	to_x = n - (move[4] - '0'); //the row, x2
	to_y = findCol(move[3]); //the col, p2

	return make_tuple(from_x, from_y, to_x, to_y);
}

void Board::undoMove(stack<array<string, 64>>& history, array<string, 64>& board, string& move, unordered_multiset<string>&Mb, unordered_multiset<string>&Mw) {
	//history.pop();
	if (!history.empty()) {
		board = history.top();
		history.pop();

		switchPlayer();

		// delete stored moves played to decrease count
		if (color == 'b') {
			Mb.erase(move);
		}
		else if (color == 'w') {
			Mw.erase(move);
		}
	}
	else {
		cout << "\n[Error Occured!]" << endl;
		return;
	}
}

bool Board::isLegalMove(int x, int y, int xx, int yy, char clr) {

	bool are_within_Dim = ((x >= 0 && x < n) && (xx >= 0 && xx < n));// 1.1 numbers are within dimention chosen
	bool are_not_same_piece = !((x == xx) && (y == yy));// 1.2 check dest != source
	char src = board[x][y];// 1.3 check if it matches the color the user entered
	char dst = board[xx][yy];// 1.4 check if dst is empty 
	// 1.5 check if it is not a blocked place (has 'x')
	// 1.6 CHECK DISTANCE
	if (are_within_Dim && are_not_same_piece && src == clr && dst == '-' && dst != 'x' && (abs(x - xx) <= 2) && (abs(y - yy) <= 2)) {
		return true;
	}
	return false;
}
bool Board::isCloneMove(int x1, int y1, int x2, int y2) {
	// 1.7. check if its a duplicate or jump move (dest blob is one on of the nieghbours of src blob)
	bool is_niehgbour = FindNeighbors(x1, y1, x2, y2);
	if (is_niehgbour) {
		return true;
	}
	return false;
}
void Board::MakeLegalMoves(unsigned int& fb, unsigned int& fw, unordered_multiset<string>& Mb, unordered_multiset<string>& Mw, char color, string move, array<string, 64>& board, string& JumpsAllowed, stack<array<string, 64>>& history) {

	//setColor(color);
	char opopent_color = (color == 'b') ? 'w' : 'b';
	vector<tuple<int, int, int, int>> blob_moves;

	if (game_over == true) {
		return;
	}
	/*load board and check if we have a board
	promt user to make a board if none*/
	if (board.empty()) {
		cout << "Please set up your board first" << endl;
		return;
	}
	history.push(board);//save move for first redo

	if (color == 'b') {
		Mb.insert(move);//store black moves
		fb = RepetitionCount(move, Mb);//check for repititon
	}
	else if (color == 'w') {
		Mw.insert(move);//store white moves
		fw = RepetitionCount(move, Mw);//check for repititon
	}
	//parse the move
	int x1, y1, x2, y2;
	tie(x1, y1, x2, y2) = parseMove(move);

	bool right_format = false;
	if (move[0] == 'm') {
		right_format = true;
	}
	bool legal = isLegalMove(x1, y1, x2, y2,color);
	bool isClone = isCloneMove(x1, y1, x2, y2);
	if (legal && right_format) {//check type of move (duplicate/jump) //keep count of jumps
		//1. change dest to player color
		board[x2][y2] = board[x1][y1];
		//1.1 update hash
		current_hash ^= ZobristTable[x1][y1][indexOf(board[x2][y2])];
		//2. if any of the nieghbours has oponent color && is not blocked change oponents color to player color;
		int rowLimit = (n * n) - 1, columnLimit = rowLimit ? n - 1 : 0;
		for (int i = std::max(0, x2 - 1); i <= std::min(x2 + 1, rowLimit); i++) {
			for (int j = std::max(0, y2 - 1); j <= std::min(y2 + 1, columnLimit); j++) {
				if (i != x2 || j != y2) {
					if (i < 0 || i >= n || j < 0 || j >= n) {
						continue;
					}
					if (board[i][j] != 'x' && board[i][j] == opopent_color) {
						board[i][j] = board[x1][y1];
						current_hash ^= ZobristTable[x1][y1][indexOf(board[i][j])];
					}
				}
			}
		}
		setBestMove(move);
	}
	else {
		return;//do not save board (pass) if illegal move
	}
	if (isClone == false) {
		board[x1][y1] = '-';
		current_hash ^= ZobristTable[x1][y1][indexOf(board[x2][y2])];
		JumpsAllowed.append("j");//append "j" to jumpsAllowed string to keep track on jumps made
	}
	history.push(board); //save history after the move
}

int Board::getScore(char c) {
	//score of player = number of player pieces on board
	int score = 0;
	for (auto& row : board) {
		for (auto& s : row) {
			if (s == c) {
				score++;
			}
		}
	}
	return score;
}

int Board::getValue() {
	char oponent_color = (color == 'b') ? 'w' : 'b';
	return getScore(color) - getScore(oponent_color);
}

vector<tuple<int, int, int, int>> Board::getLegalMoves(char c) {
	//setColor(c);
	int x, y, xx, yy;
	vector<tuple<int, int, int, int>> legalMoves;

	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (board[i][j] == c) {
				x = i;
				y = j;
				//cout << "\nx y : " << x << y << endl;

				for (int k = i - 2; k < i + 3; k++) {
					for (int l = -2; l < j + 3; l++) {
						xx = k;
						yy = l;
						//cout << "\nxx yy: " << xx << yy << endl;
						if (xx < 0 || yy < 0 || xx >= n || yy >= n) {
							continue;//skip negative & out of bounds indices
						}
						if (isLegalMove(x, y, xx, yy,c)) {
							legalMoves.emplace_back(x, y, xx, yy);
						}
					}
				}
			}
		}
	}
	/*cout <<"\nthe legal moves of: "<<c << endl;
	for (const auto& i : legalMoves) {
	  cout << get<0>(i) << ", " << get<1>(i) << ", " << get<2>(i)<< ", " << get<3>(i) << endl;
	}*/
	return legalMoves;
}

bool Board::nolegalMovesLeft() {
	//check whether both players have no legal moves left (for ending game)
	vector<tuple<int, int, int, int>> black;
	vector<tuple<int, int, int, int>> white;

	white = getLegalMoves('w');
	black = getLegalMoves('b');

	if (black.size() == 0 && white.size() == 0) {
		return true;
	}
	return false;
}

vector<pair<string, unsigned long long int>> Board::getChildren(char c) {
	
	vector<pair<string, unsigned long long int>> children;
	int x1, y1, x2, y2;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (board[i][j] == c) {
				x1 = i;
				y1 = j;
				for (int k = i - 2; k < i + 3; k++) {
					for (int l = j - 2; l < j + 3; l++) {
						x2 = k;
						y2 = l;
						if (x2 < 0 || y2 < 0 || x2 >= n || y2 >= n) {
							continue;//skip negative indices
						}
						if (isLegalMove(x1, y1, x2, y2,c)) {
							string c_move = "m" + ReconstructMove(x1, y1, x2, y2);
							if (HH.find(c_move) == HH.end()) { HH.emplace(c_move, 0); }
							children.emplace_back(make_pair(c_move, HH[c_move]));
						}
					}
				}
			}
		}
	}
	//cout << "\nchildren of: " <<color<<endl; //testing children
	//for(const auto &i : children){
	//	cout <<"Move: "<<i.first << " order: " << i.second<<endl;
	//}
	return children;
}

int Board::findCol(char p)
{//helper function 1
	int pos;
	switch (p) {
	case 'a': pos = 0; break;
	case 'b': pos = 1; break;
	case 'c': pos = 2; break;
	case 'd': pos = 3; break;
	case 'e': pos = 4; break;
	case 'f': pos = 5; break;
	case 'g': pos = 6; break;
	case 'h': pos = 7; break;
	}
	return pos;
}

string Board::findRow(int x)
{//helper function 2
	string pos;
	switch (x) {
	case 0: pos = "a"; break;
	case 1: pos = "b"; break;
	case 2: pos = "c"; break;
	case 3: pos = "d"; break;
	case 4: pos = "e"; break;
	case 5: pos = "f"; break;
	case 6: pos = "g"; break;
	case 7: pos = "h"; break;
	}
	return pos;
}

bool Board::FindNeighbors(int x, int y, int to_x, int to_y)
{
	int rowLimit = (n * n) - 1, columnLimit = rowLimit ? n - 1 : 0;
	for (int i = std::max(0, x - 1); i <= std::min(x + 1, rowLimit); i++) {
		for (int j = std::max(0, y - 1); j <= std::min(y + 1, columnLimit); j++) {
			if (i != x || j != y) {
				if (to_x == i && to_y == j) {
					return true;
				}
			}
		}
	}
	return false;
}

string Board::ReconstructMove(int x1, int y1, int x2, int y2) {
	//ex:move 0302 = d3c4 since rows are in decending order
	/*string xx1 = findRow(n - x1 - 1);
	string xx2 = findRow(n - x2 - 1);
	return xx1 + std::to_string(y1 + 1) + xx2 + std::to_string(y2 + 1);*/

	string yy1 = findRow(y1);
	string yy2 = findRow(y2);
	return yy1 + std::to_string(n-x1) + yy2 + std::to_string(n-x2);
}

void Board::print_score() {
	int black_score = getScore('b');
	int white_score = getScore('w');
	cout << "\nSCORE: " << black_score << " (b) " << white_score << " (w)" << endl;
	if (game_over){
		if (black_score > white_score) {
			cout << "## Black wins! " << endl;
			cout << "Game Over: " << "## " << black_score << " - () " << white_score << " = " << black_score - white_score;
		}
		else if (white_score > black_score) {
			cout << "() white wins! " << endl;
			cout << "Game Over: " << "() " << white_score << " - ## " << black_score << " = " << white_score - black_score;
		}
		else {
			cout << "its a draw !" << endl;
			cout << "Game Over: " << "## " << black_score << " - () " << white_score << " = " << black_score - white_score;
		}
	}
}

bool Board::gameOver() {
	string maxJumps;
	maxJumps.insert(0, 50000, 'j'); //50k for safety inside search just in case

	//1. check if both b & w has no legal moves
	bool no_moves = nolegalMovesLeft();
	//2.check for 3 repetition rule for black and white
	bool repetition_occured = (fb == 4 || fw == 4) ? true : false; //must =4 cuz the function count the move as well 
	//3.check if 50 jumps are made in a row
	bool ReachedMaxJumps = JumpsAllowed.find(maxJumps) != string::npos;

	//cout << "\nno_moves: " << no_moves << endl;
	//cout << "repetition: " << repetition_occured << endl;
	//cout << "reached max jump: " << ReachedMaxJumps << endl;

	//check if game is over and print result (4. game also ends if a player lost all his pieces)
	if (ReachedMaxJumps|| no_moves|| repetition_occured || (getScore('w') == 0 || getScore('b') == 0)){
		game_over = true;
	}
	else {
		game_over = false;
	}
	
	return game_over;
}

int Board::RepetitionCount(string move, unordered_multiset<string> SavedMoves)
{
	/*
how to works:
	s1:a1b1 -> move
	s2:b1a1 -> first comeback to old position (found=1)
	s1:a1b1 ->second comeback to old position (found=2)
	s2:b1a1 -> third comeback to old position (found=3)

	this function counts the occurance of the move as well;
	for the 3 rule repetetition, found must be = 4 since function counts the first move as well
*/
	string f1(1, move[1]), f2(1, move[2]), t1(1, move[3]), t2(1, move[4]);
	string s1 = "m" + f1 + f2 + t1 + t2;
	string s2 = "m" + t1 + t2 + f1 + f2;

	return SavedMoves.count(s1) + SavedMoves.count(s2);
}

//--------------------------------------------------HASH----------------------
unsigned long long int Board::getHash()const {
	return current_hash;
	//value of hash will not change in func 
}
int Board::indexOf(char c) {
	if (c == 'b') { return 1; }
	else { return 0; }
}
unsigned long long int Board::randomInt() {
	// Generates a Randome number from 0 to 2^64-1 
	uniform_int_distribution<unsigned long long int> dist(0, UINT64_MAX);
	return dist(mt);
}
unsigned long long int Board::computeHash(array<string, 64>& board) {
	// Computes the hash value of a given board  
	unsigned long long int current_hash = 0;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			if (board[i][j] != '-' && board[i][j] != 'x') {
				int piece = indexOf(board[i][j]);
				current_hash ^= ZobristTable[i][j][piece];
			}
		}
	}
	return current_hash;
}
void Board::initTable(int n) {
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			for (int k = 0; k < 2; k++)
				ZobristTable[i][j][k] = randomInt();
}
TTEntry& Board::checkTT(const unsigned long long int& _hash, int depth, int& alpha, int& beta)const {

	TTEntry& e = TT[_hash % 256000]; //ref e = hashTable[h % hashTable.size()]

	if (e.hash == _hash && e.depth >= depth) {
		if (e.f == EXACT) { alpha = beta = e.value; }
		else if (e.f == LOWERBOUND) { if (alpha < e.value) { alpha = e.value; } }
		else if (e.f == UPPERBOUND) { if (beta > e.value) { beta = e.value; } }
	}
	return e;
}
void Board::StoreEntry(TTEntry& e, unsigned long long& _hash, int& score, int& depth, int& alpha, int& beta, string& _bestMove, unsigned long long int& best_move_hash) const {
	// also store hash after best move played.
	if (e.depth > depth) { return; }
	e.hash = _hash;
	e.value = score;
	e.bestMove = _bestMove;
	e.depth = depth;
	e.best_move_hash = best_move_hash;
	if (score <= alpha) { e.f = UPPERBOUND; }
	else if (score >= beta) { e.f = LOWERBOUND; }
	else { e.f = EXACT; }
}
bool Board::SortChild(const pair<string, unsigned long long int>& a, const pair<string, unsigned long long int>& b) {
	return a.second > b.second;
}
//int Board::AlphaBeta(int depth, char color, array<string, 64>& board, int alpha, int beta, stack<array<string, 64>>& history) {
//	//first ABsearch version
//	record.calls++; //get no. of function calls
//	// evaluate leaf position from current player’s standpoint 
//	if (depth == 0 || game_over == true) { return getValue(); }//when game is over a node was terminal
//	unsigned long long int child_hash = getHash();
//	TTEntry& e = checkTT(child_hash, depth, alpha, beta);
//	if (alpha >= beta) { record.TTC++; return e.value; }
//
//	vector<pair<string, unsigned long long int>> children = getChildren(color);// generate successor moves (all legal move nodes)
//	if (children.size() == 0) { return getValue(); }//make sure there are legal moves
//
//	// try heuristically best move first (get best move from table if found)
//	if (e.hash == child_hash) {
//		for (unsigned int i = 0; i < children.size(); i++) {
//			if (children[i].first == e.bestMove) {
//				best_move = e.bestMove;
//				children[i].second = 0;//for move ordering
//				break;
//			}
//		}
//	}
//	int score = -100;
//	bool cut = false;
//	unsigned long long int best_hash = 0;// best move hash
//	unsigned long long int temp_hash;
//
//	sort(children.begin(), children.end(), SortChild);// Move Ordering
//
//	for (unsigned int i = 0; i < children.size(); i++) {
//		MakeLegalMoves(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);// execute current move
//
//		int value = -AlphaBeta(depth - 1, color, board, -beta, -alpha, history);//call other player, and switch sign of returned value
//		temp_hash = getHash();
//		if (value > score) {// compare returned value and score value, note new best score if necessary
//			best_move = children[i].first; // update best move if necessary
//			best_hash = temp_hash;
//			score = value;
//		}
//		undoMove(history, board, children[i].first, Mb, Mw);// retract current move 
//
//		if (score > alpha) { alpha = score; }//adjust the search window
//		if (alpha >= beta) {//cut off
//			record.Cut_BF++;
//			record.successors += 1 + i;//+i we get the sucessor for each iteration
//			cut = true;
//			break;
//		}
//	}
//	if (cut == true) { HH[best_move] += 2 ^ depth; }// update history score
//	StoreEntry(e, child_hash, score, depth, alpha, beta, best_move, best_hash);
//	return score;
//}

int Board::AlphaBeta(int depth, char color, array<string, 64>& board, int alpha, int beta, stack<array<string, 64>>& history) {
	
	//code for AlphaBeta & negascout is referenced from the course notes + references in README.txt
   //naming as per a1.txt
	//version 2 of ABsearch

	record.calls++; //get no. of function calls
	// evaluate leaf position from current player’s standpoint 
	if (depth == 0 || game_over == true) { return getValue(); }//when game is over a node was terminal

	unsigned long long int child_hash = getHash();

	TTEntry& e = checkTT(child_hash, depth, alpha, beta);
	if (alpha >= beta) { record.TTC++; return e.value; }

	vector<pair<string, unsigned long long int>> children = getChildren(color);// generate successor moves (all legal move nodes)
	if (children.size() == 0) { return getValue(); }//make sure there are legal moves

	string temp_best_move = "ms1s2";
	int move_idx = -1;
	int start_idx = -1;
	int start_score = -100;

	// try heuristically best move first (get best move from table if found)
	if (e.hash == child_hash) {
		for (unsigned int i = 0; i < children.size(); i++) {
			if (children[i].first == e.bestMove) {
				temp_best_move = e.bestMove;
				move_idx = i;
				children[i].second = 0;//for move ordering
				break;
			}
		}
	}
	//find the best move when d=1 (if not found on table)
	for (unsigned int i = 0; i < children.size() && children[i].first != temp_best_move; i++) {
		//cout << "\nmove trying :" << children[i].first << endl;
		MakeLegalMoves(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);//excute current move
		
		int v = -getValue();

		if (start_score < v) {
			start_score = v;
			start_idx = i;
		}
		//undo bad move
		undoMove(history, board, children[i].first,Mb,Mw);

	}

	int score = -100;
	bool cut = false;
	unsigned long long int best_hash = 0;// best move hash
	unsigned long long int temp_hash;
	string child_best_move = "";
	int new_alpha = alpha;//copy alpha

	while (true) {
		int value;

		if (move_idx != -1) {//get best move
			
			MakeLegalMoves(fb, fw, Mb, Mw, color, children[move_idx].first, board, JumpsAllowed, history);

			temp_hash = getHash();
			// assumes alternating moves
			value = -AlphaBeta(depth - 1, color, board, -beta, -new_alpha, history);//call other player, and switch sign of returned value

			//undo bad move
			undoMove(history, board, children[move_idx].first, Mb, Mw);

			if (value > score) {
				child_best_move = children[move_idx].first; //update move if necessary
				best_hash = temp_hash;
				score = value;

				if (score > new_alpha) { new_alpha = score; }
				if (score >= beta) {//adjust the search window
					record.Cut_BF++;
					record.successors += 1;//when a cut occurs its a leaf node (used for CBF average)
					cut = true;
					break;
				}
			}
		}
		// get best value move
		if (start_idx != -1) {
			MakeLegalMoves(fb, fw, Mb, Mw, color, children[start_idx].first, board, JumpsAllowed, history);

			value = -AlphaBeta(depth - 1, color, board, -beta, -new_alpha, history);
			temp_hash = getHash();

			//undo
			undoMove(history, board, children[start_idx].first, Mb, Mw);

			if (value > score){ // compare returned value and score value, note new best score if necessary
				child_best_move = children[start_idx].first;

				//cout << "\nchild_best_move: " << child_best_move << endl; //test
				best_hash = temp_hash;
				score = value;
				if (score > new_alpha) { new_alpha = score; }
				if (score >= beta) {
					record.Cut_BF++;
					cut = true;
					record.successors += 1;
					break;
				}
			}
		}
		if (start_idx != -1) {
			//assign lower values for found moves and stop getting best move
			children[start_idx].second = 0;
		}
		//sort the moves
		sort(children.begin(), children.end(), SortChild);
		for (unsigned int i = 0; i < children.size(); i++) {
			//play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
			MakeLegalMoves(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);

			value = -AlphaBeta(depth - 1, color, board, -beta, -new_alpha, history);
			temp_hash = getHash();

			//undo
			undoMove(history, board, children[i].first, Mb, Mw);

			if (value > score) {
				child_best_move = children[i].first;
				best_hash = temp_hash;
				score = value;
				if (score > new_alpha) { new_alpha = score; }
				if (score >= beta) {
					record.Cut_BF++;
					record.successors += 1 + i;//+i we get the sucessor for each iteration
					cut = true;
					break;}
			}
		}
		break;
	}
	//setMove(child_best_move);
	if (cut == true) { HH[child_best_move] += 2 ^ depth; } //update history store
	StoreEntry(e, child_hash, score, depth, alpha, beta, child_best_move, best_hash);

	return score;
}
int Board::NegaScout(int depth, char color, array<string, 64>& board, int alpha, int beta, stack<array<string, 64>>& history) {

	record.calls++;

	//negascout search (called with mode 2)
	//NegaScout will find best two moves then start negascout search
	//other parts are similar to alphabeta search
	//naming here is also similar to the class notes

	setColor(color);
	function_calls++;//calls global counter
	if (depth == 0 || game_over == true) { return getValue(); }//when game is over a node was terminal
	unsigned long long int child_hash = getHash();
	TTEntry& e = checkTT(child_hash, depth, alpha, beta);
	record.TTQ++;
	if (alpha >= beta) { record.TTC++; return e.value; }

	vector<pair<string, unsigned long long int>> children = getChildren(color);//all children of current player
	if (children.size() == 0) { return getValue(); }

	string table_best_move = "ms1s2";
	string start_move = "ms1s2";
	int move_idx = -1;
	int start_idx = -1;
	int start_score = -100;

	if (e.hash == child_hash) {
		for (unsigned int i = 0; i < children.size(); i++) {
			if (children[i].first == e.bestMove) {
				table_best_move = e.bestMove;
				move_idx = i;
				children[i].second = 0;
				break;
			}
		}
	}
	for (unsigned int i = 0; i < children.size() && children[i].first != table_best_move; i++) {
		//play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
		MakeLegalMoves(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);

		int v = -getValue();
		if (start_score < v) {
			start_score = v;
			start_idx = i;
		}
		//undo bad move
		undoMove(history, board, children[i].first, Mb, Mw);
	}
	int score = -100;
	bool cut = false;
	unsigned long long int best_hash = 0;// best move hash
	unsigned long long int temp_hash;
	string child_best_move = "";
	int new_alpha = alpha;//copy alpha

	while (true) {
		int value;
		if (move_idx != -1) {
			//play(fb, fw, Mb, Mw, color, children[move_idx].first, board, JumpsAllowed, history);
			MakeLegalMoves(fb, fw, Mb, Mw, color, children[move_idx].first, board, JumpsAllowed, history);

			value = -NegaScout(depth - 1, color, board, -beta, -new_alpha, history);
			temp_hash = getHash();
			undoMove(history, board, children[move_idx].first, Mb, Mw);
			if (value > score) {
				best_hash = temp_hash;
				child_best_move = children[move_idx].first;
				score = value;
				if (score > new_alpha) { new_alpha = score; }
				if (score >= beta) {
					record.Cut_BF++;
					record.successors += 1;
					cut = true; break;
				}
			}
		}
		if (start_idx != -1) {
			//play(fb, fw, Mb, Mw, color, children[start_idx].first, board, JumpsAllowed, history);
			MakeLegalMoves(fb, fw, Mb, Mw, color, children[start_idx].first, board, JumpsAllowed, history);

			value = -NegaScout(depth - 1, color, board, -beta, -new_alpha, history);
			temp_hash = getHash();
			undoMove(history, board, children[start_idx].first, Mb, Mw);
			if (value > score) {
				best_hash = temp_hash;
				child_best_move = children[start_idx].first;
				score = value;
				if (score > new_alpha) { new_alpha = score; }
				if (score >= beta) {
					record.Cut_BF++;
					cut = true;
					record.successors += 1;
					break;
				}
			}

		}
		if (start_idx != -1) {
			children[start_idx].second = 0;
		}
		sort(children.begin(), children.end(), SortChild);

		//play(fb, fw, Mb, Mw, color, children[0].first, board, JumpsAllowed, history);
		MakeLegalMoves(fb, fw, Mb, Mw, color, children[0].first, board, JumpsAllowed, history);

		value = -NegaScout(depth - 1, color, board, -beta, -new_alpha, history);
		temp_hash = getHash();
		undoMove(history, board, children[0].first, Mb, Mw);
		if (value > score) {
			best_hash = temp_hash;
			child_best_move = children[0].first;
			score = value;
			if (score > new_alpha) { new_alpha = score; }
			if (score >= beta) { record.Cut_BF++; cut = true; record.successors += 1; break; }
		}
		if (score < beta) {
			for (unsigned int i = 1; i < children.size(); ++i) {
				int lower_bound = max(new_alpha, score);
				int upper_bound = lower_bound + 1;
				//play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
				MakeLegalMoves(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);

				int s = -NegaScout(depth - 1, color, board, -upper_bound, -lower_bound, history);
				temp_hash = getHash();
				undoMove(history, board, children[i].first, Mb, Mw);
				if (s >= upper_bound && s < beta) {
					//play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
					MakeLegalMoves(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);

					s = -NegaScout(depth - 1, color, board, -beta, -s, history);
					temp_hash = getHash();
					undoMove(history, board, children[i].first, Mb, Mw);
				}
				if (s > score) {
					best_hash = temp_hash;
					child_best_move = children[i].first;
					score = s;
				}
				if (score >= beta) {
					cut = true;
					break;
					record.Cut_BF++;
					record.successors += 1 + i;//+i we get the sucessor for each iteration
				}
			}
		}
		break;
	}
	if (cut == true) { HH[child_best_move] += 2 ^ depth; }
	StoreEntry(e, child_hash, score, depth, alpha, beta, child_best_move, best_hash);

	return score;
}
bool Board::time_over(std::chrono::time_point<std::chrono::system_clock> startTime) {
	std::chrono::duration<double> D = (std::chrono::system_clock::now() - startTime);
	if (D.count() >= search_time_rt * 0.85 || D.count() >= search_time_ft) {
		return true;
	}
	return false;
}
//search 
void Board::Search(int d, int alpha, int beta, std::chrono::time_point<std::chrono::system_clock> startTime) {
	//iteradive deapening search throughout depths 

	int score = 0;
	unsigned long long int bestHash;

	//initialize search parameters at the beginig of every new search
	record.calls = 0;
	record.Cut_BF = 1;
	record.successors = 0;
	record.TTC = 0;
	record.TTF = 0;
	record.TTQ = 0;

	const int WIDTH = 11;
	//--------------------------HEADER
	cout << left << setw(WIDTH) << setfill(' ') << "CTM";
	cout << left << setw(WIDTH) << setfill(' ') << "D";
	cout << left << setw(WIDTH) << setfill(' ') << "Time";
	cout << left << setw(WIDTH) << setfill(' ') << "Calls";
	cout << left << setw(WIDTH) << setfill(' ') << "Speed";
	cout << left << setw(WIDTH) << setfill(' ') << "TTQ";
	cout << left << setw(WIDTH) << setfill(' ') << "TTF";
	cout << left << setw(WIDTH) << setfill(' ') << "TTC";
	cout << left << setw(WIDTH) << setfill(' ') << "CBF";
	cout << left << setw(WIDTH) << setfill(' ') << "Value" << endl;

	for (int i = 1; i <= d; i++) {
		// during search AI playes two moves and print result and borad
		if (search_mode == 1) {
			score = AlphaBeta(i, color, board, alpha, beta, history);
		}
		else if (search_mode == 2) {
			score = NegaScout(i, color, board, alpha, beta, history);
		}
		std::chrono::duration<double> time = std::chrono::system_clock::now() - startTime;

		if (time_over(startTime) ) {
			search_results(time, score, i);
			break;
		}
		else {
			bestHash = getHash();
			best_move = TT[bestHash % 256000].bestMove;
		}
		search_results(time, score, i);
	}
	//MakeLegalMoves(fb, fw, Mb, Mw, color, best_move, board, JumpsAllowed, history);// execute current move if not made
	cout << "\nmove: " << best_move.substr(1, 4) << endl;
	cout << "search score: " << score << endl;
}
//print result of search
void Board::search_results(std::chrono::duration<double> time, int _score, int i) {
	//time: total time used so far for all iterations in seconds
	//speed:# of Alpha-Beta calls / total time so far
	//Cut - BF: The average number of successors considered in states in which a cut - off occurs.Count only nodes where at least one move
	//i: number of times

	record.CTM = (color == 'b') ? "##" : "()";
	const int WIDTH = 11;

	cout << fixed << showpoint;
	cout << setprecision(3);

	//------------------------PRINT VALUES
	cout << left << setw(WIDTH) << setfill(' ') << record.CTM;
	cout << left << setw(WIDTH) << setfill(' ') << i;
	cout << left << setw(WIDTH) << setfill(' ') << time.count();
	cout << left << setw(WIDTH) << setfill(' ') << record.calls;
	cout << left << setw(WIDTH) << setfill(' ') << record.calls / time.count();
	cout << left << setw(WIDTH) << setfill(' ') << record.TTQ;
	cout << left << setw(WIDTH) << setfill(' ') << record.TTF;
	cout << left << setw(WIDTH) << setfill(' ') << record.TTC;
	cout << left << setw(WIDTH) << setfill(' ') << record.successors / record.Cut_BF;
	cout << left << setw(WIDTH) << setfill(' ') << _score << endl;

	//cout << "\nCTM\t D\t Time\t      Calls\t    Speed\t    TTQ\t    TTF\t    TTC\t    CBF\t    Value" << endl;
	//cout << record.CTM << "\t " << i << "\t " << time.count() 
	   //  << "\t" << record.calls << "\t    " << record.calls / time.count() << "\t    "
	   //  << record.TTQ << "\t    " << record.TTF << "\t    " <<record.TTC<<"\t    " 
	   //  << record.successors/record.Cut_BF << "\t    " << _score;
}

//--------------------------------------------Start GAME Function------------------------------------
void Board::Game() {

	string UserInput;
	//promt user to initialize the board
	cout << "Inter (i ) followed by the size of the board to initialize the game or (q) to exit at any time" << endl;
	while (true) {
		cout << "Initialize Board: ";
		getline(cin, UserInput);
	START:
		int dimention = UserInput[2] - '0';
		if (UserInput == "s" || UserInput == "S") { //make sure user is not creating board before initialization
			cout << "[i command must be given before s]" << endl;
			continue;
		}
		if (dimention < 4 || dimention > 8) {//check if the n is within allowed dimention (4,8)
			cout << "[Please enter dimention betwewn 4 and 8]" << endl;
			continue;
		}
		string valid_init = "i ";
		//Check if user is entering the right command [i n]
		if (UserInput != valid_init.append(to_string(dimention))) {
			cout << "[Invalid initialization]: " << UserInput << endl;
			continue;
		}
		else { break; }
	}
	setSize(UserInput);
	cout << "[set board to size " << n << "]" << endl;

	//display default board upon initilization for testing 
	//DisplayBoard();

	string cmd;

MENUE:
	while (CMD != "q") {
		cout << "\nEnter cmd: ";
		getline(cin, cmd);
		setCMD(cmd);

		if (CMD[0] == 's') {
			//need to clear history if s is interred
			game_over = false;
			//initialize the table
			initTable(n);
			while (!history.empty()) {
				history.pop();
			}
			getBoard(board);//create the board	
			cout << "New Board: " << endl;
			PrintBoard();
			history.push(board);
			continue;
		}
		else if (CMD[0] == 'i' && CMD[1] == ' ' && isdigit(CMD[2])) {
			UserInput = CMD;
			goto START;
		}
		else if (CMD[0] == 'b') {
			setColor('b');
			cout << "\n[Black is to move ]" << endl;
			continue;
		}
		else if (CMD[0] == 'w') {
			setColor('w');
			cout << "\n[white is to move ]" << endl;
			continue;
		}
		else if (CMD[0] == 'u') {
			if (history.empty()) {
				cout << "[No previous move available]" << endl;
			}
			else {
				cout << "\n[Undoing your latest move... ]" << endl;
				history.pop();
				undoMove(history, board,move, Mb, Mw);
				PrintBoard();
			}
			cout << "\n[" << color << " is to move]" << endl;
			continue;
		}
		else if (CMD[0] == '1') {
			setMode(1);
			cout << "\n[Mode 1]" << endl;
			continue;
		}
		//check the input after the mode choice
		else if (CMD[0] == 'f' && CMD[1] == 't' && CMD[2] == ' ') { //set fixed search time
			double time = 0; //to avoid loss of data
			for (unsigned int t = 3; t < CMD.size(); t++) {
				if (CMD[t] >= '0' && CMD[t] <= '9') {
					time = time * 10 + (CMD[t] - '0');
				}
				else { break; }
			}
			cout << "[fixed search time " << time << " seconds]" << endl;
			setTime(time);
			continue;
		}
		else if (CMD[0] == 'd' && CMD[1] == ' ') {//set search depth
			int _depth = 0;
			for (unsigned int d = 2; d < CMD.size(); d++) {
				if (CMD[d] >= '0' && CMD[d] <= '9') {
					_depth = _depth * 10 + (CMD[d] - '0');
				}
				else { break; }
			}
			setDepth(_depth);
			cout << "[search depth " << depth << "]" << endl;
			continue;
		}
		else if (CMD[0] == '2') {
			setMode(2);
			cout << "\n[Mode 2]" << endl;
			continue;
		}
		// get rt for the *entire* game for each player
		else if (CMD[0] == 'r' && CMD[1] == 't' && CMD[2] == ' ') {
			double time = 0;
			for (unsigned int t = 3; t < CMD.size(); t++) {
				if (CMD[t] >= '0' && CMD[t] <= '9') {
					time = time * 10 + (CMD[t] - '0');
				}
				else { break; }
			}
			cout << "[remaining time " << time << " seconds]" << endl;
			setTime(time);
			continue;
		}
		else if (CMD[0] == 'm' && CMD.size() == 5 ) {
			
			if (game_over) {
				cout << "[game is finished, no further moves accepted]" << endl;
				goto MENUE;
			}
			else {
				setMove(CMD);
				//check if move has alpha num for parse to work 
				if ((isdigit(move[2]) && isdigit(move[4])) && isalpha(move[1]) && isalpha(move[3])) {
					int x1, y1, x2, y2;
					tie(x1, y1, x2, y2) = parseMove(move);
					if (isLegalMove(x1, y1, x2, y2, color)) {
						MakeLegalMoves(fb, fw, Mb, Mw, color, move, board, JumpsAllowed, history);
						PrintBoard();
						print_score();
						switchPlayer();
						cout << "\n[" << color << " is to move]" << endl;
					}
				}
				else {
					cout << "\n[illegal move]" << endl;
				}	
			}
			continue;
		}
		else if (CMD[0] == 'q') { cout << "\nQuitting Game.. " << endl; exit(0); }

		else if (CMD[0] == 'g') {
			goto SEARCH;//start search
		}
		else { cout << "\n[Invalid Command]" << endl; }
	}


SEARCH:
	//search start at this tag:
	//start search, print results, player switch, go back to get user input again

	if (game_over==true) {
		cout << "[game is finished, no further moves accepted]" << endl;
		goto MENUE;
	}
	//check if there are legal moves
	//vector<tuple<int, int, int, int>> blob_moves = getLegalMoves(color);
	else if (getLegalMoves(color).size() == 0) { // pass player if he has no legal moves left
		string pass_move = "a1a1";
		setMove(pass_move);
		cout << "move: " << move << endl;
		gameOver();
		switchPlayer();
		print_score();
		PrintBoard();
		cout << "\n[" << color << " is to move]" << endl;
		goto MENUE;
	}
	//check if we need ft or rt for prenting startimg info
	else if (TimeFlag == 2) {
		//we have rt set (show ft for each move till the end of search )
		cout << "FT: " << search_time_ft << endl;
	}
	else {//we have ft set , set a fixed time for each player per move per game
	   //we get FT based on empty spaces left on board
		//print starting time settings at begining of search(saves previous output when 'g' is enetered again)
		search_time_ft = search_time_rt * 0.9 / (getScore('-') + 1);
		cout << "RT: " << search_time_rt + extra_time << endl;
		cout << "Total time used: " << total_time << endl;
		cout << "Allocated: " << search_time_ft << endl;
	}

	if (TimeFlag == 2) {
		//check if we are exceeding time limit
		if (search_time_rt <= 0) {
			cout << "Search over, Ending Game..." << endl;
			game_over = true;
			print_score();
			switchPlayer();
			cout << "\n[" << color << " is to move]" << endl;
			goto MENUE;
		}
	}
	
	std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();

	//start search (iterative depening)
	Search(depth, -100, 100, startTime);

	//--------search() only makes best moves (check if no best moves found)---
	//check if child's best move is still temp_best_move s1s2 (or no best move)
	if (best_move == "ms1s2" || best_move.empty()) {
		vector<pair<string, unsigned long long int>> children = getChildren(color);
		if (children.size() != 0) {
			best_move = children[0].first;
			MakeLegalMoves(fb, fw, Mb, Mw, color,best_move, board, JumpsAllowed, history);
			cout << "move: " << best_move.substr(1, 4) << endl;
		}
	}

	//end search
	std::chrono::duration<double> D = (std::chrono::system_clock::now() - startTime);

	double used_time = D.count();
	search_time_rt -= used_time;
	total_time += used_time;
	if (TimeFlag == 1) {
		cout << "=======================TIME RESULT=============================" << endl;
		cout << "RT: " << search_time_rt + extra_time << endl;
		cout << "Total Time Used: " << total_time << endl;
		cout << "Time Used: " << used_time << endl;
	}
	gameOver();//check if game is over
	//switchPlayer();
	cout << "\nfinal board:" << endl;
	PrintBoard();
	print_score();
	cout << "\n[" << color << " is to move]" << endl;
	goto MENUE;

}

