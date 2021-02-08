#include "Board.h"

 int Board::n; //dimention
 array<string, 64> Board::board;
 stack<array<string, 64>> Board::history;

 char Board::color;
 string Board::move;
 unsigned int Board::fb = 0;
 unsigned int Board::fw = 0;
 unordered_multiset<string> Board::Mb;
 unordered_multiset<string> Board::Mw;
 string Board::CMD;
 bool Board::game_over = false;
 string Board::JumpsAllowed = "";
 int Board::depth = 16;

 Report Board::record;

 //..............................Search.................................
 int Board::search_mode;
 int Board::current_turn = 1;

 //...........................Timing....................................
 double Board::search_time_ft;
 double Board::search_time_rt;
 double Board::total_time=0;
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
void Board::setDepth(int _depth){
	 depth = _depth;
 }
void Board::switchPlayer(){
	if (color == 'b') {
		setColor('w');
	}
	else {
		setColor('b');
	}
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
void Board::setMove(string& m){
	move = m;
}

 void Board::DisplayBoard(){
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

 void Board::getBoard(array<string, 64>& board){
	 //accepting board line by line seperated by Enter key
	 cout << "\nEnter your Board: " << endl;
	 for (int i = 0; i < n; i++) {
		 getline(cin, board[i]);
	 }
 }
 void Board::PrintBoard(){
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

 tuple<int, int, int, int> Board::parseMove(string move){
	 int from_x = n - (move[2] - '0'); //the row in board, x1
	 int from_y = findCol(move[1]); //the col in board, p1
	 int to_x = n - (move[4] - '0'); //the row, x2
	 int to_y = findCol(move[3]); //the col, p2

	 return make_tuple(from_x, from_y, to_x, to_y);
 }

 void Board::undoMove(stack<array<string, 64>>& history, array<string, 64>& board){
	 //history.pop();
	 if (!history.empty()) {
		 board = history.top();
		 history.pop();
	 }
	 else {
		 cout << "\n[Error Occured!]" << endl;
		 return;
	 }
	 //after undo, delete stored moves played
	 if (color == 'b') {
		 Mb.erase(move);
	 }
	 else if (color == 'w') {
		 Mw.erase(move);
	 }
 }

 bool Board::isLegalMove(int x, int y, int xx, int yy){

	 bool are_within_Dim = ((x >= 0 && x < n) && (xx >= 0 && xx < n));// 1.1 numbers are within dimention chosen
	 bool are_not_same_piece = !((x == xx) && (y == yy));// 1.2 check dest != source
	 char src = board[x][y];// 1.3 check if it matches the color the user entered
	 char dst = board[xx][yy];// 1.4 check if dst is empty 
	 // 1.5 check if it is not a blocked place (has 'x')
	 // 1.6 CHECK DISTANCE
	 if (are_within_Dim &&are_not_same_piece && src == color && dst == '-' && dst != 'x' && (abs(x - xx) <= 2) && (abs(y - yy) <= 2)) {
		 return true;
	 }
	 return false;
 }
 bool Board::isCloneMove(int x1, int y1, int x2, int y2){
	 // 1.7. check if its a duplicate or jump move (dest blob is one on of the nieghbours of src blob)
	 bool is_niehgbour = FindNeighbors(x1, y1,x2, y2);
	 if (is_niehgbour) {
		 return true;
	 }
	 return false;
 }

 void Board::makeMove(bool is_Clone,int x1, int y1, int x2, int y2,array<string, 64>& board)
 {
	 //change colors of dst & oponent blobs to CMD
	 char opopent_color = (color == 'b') ? 'w' : 'b';
	 //1. change dest to player color
	 board[x2][y2] = board[x1][y1];
	 //1.1 update hash
	 current_hash ^= ZobristTable[x1][y1][indexOf(board[x2][y2])];
	 //2. if any of the nieghbours has oponent color && is not blocked change oponents color to player color;
	 int rowLimit = (n * n) - 1, columnLimit = rowLimit ? n - 1 : 0;
	 for (int i = std::max(0, x2 - 1); i <= std::min(x2 + 1, rowLimit); i++) {
		 for (int j = std::max(0, y2 - 1); j <= std::min(y2 + 1, columnLimit); j++) {
			 if (i != x2 || j != y2) {

				 /*cout << "\ninside Make move, x2 y2: " << x2 << y2 << endl;
				 cout << "\ninside Make move, i j: " << i << i << endl;
				 cout << "\ninside Make move, oponent color: " << opopent_color << endl;*/

				 if (i < 0 || i >= n || j < 0 || j >= n) {
					 continue;
				 }
				 if (board[i][j] != 'x' && board[i][j] == opopent_color) {
					 board[i][j] = board[x1][y1];
					 current_hash ^= ZobristTable[x1][y1][indexOf(board[i][j])];
				 }
				 if (is_Clone == false) {
					 board[x1][y1] = '-';
				 }
			 }
		 }
	 }
 }

 int Board::getScore(char color){
	 //score of player = number of player pieces on board
	 int score = 0;
	 for (auto& row : board) {
		 for (auto& s : row) {
			 if (s == color) {
				 score++;
			 }
		 }
	 }
	 return score;
 }

 int Board::getValue(){
	 char oponent_color = (color == 'b') ? 'w' : 'b';
	 return getScore(color) - getScore(oponent_color);
 }

 vector<tuple<int, int, int, int>> Board::getLegalMoves(char c){
	 setColor(c);
	 int x, y, xx, yy;
	 vector<tuple<int, int, int, int>> legalMoves;

	 for (int i = 0; i < n; i++) {
		 for (int j = 0; j < n; j++) {
			 if (board[i][j] == c) {
				 x = i;
				 y = j;
				 //cout << "\nx y : " << x << y << endl;

				 for (int k =i - 2; k <i+ 3; k++) {
					 for (int l = - 2; l <j+ 3; l++) {
						 xx = k;
						 yy = l;
						 //cout << "\nxx yy: " << xx << yy << endl;
						 if (xx < 0 || yy < 0|| xx>=n ||yy>=n) {
							 continue;//skip negative & out of bounds indices
						 }
						 if (isLegalMove(x, y, xx, yy)) {
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

 bool Board::nolegalMovesLeft(){
	 //check whether both players have no legal moves left (for ending game)
	 vector<tuple<int, int, int, int>> black;
	 vector<tuple<int, int, int, int>> white;

	 white = getLegalMoves('w');
	 black = getLegalMoves('b');

	 if (black.size()==0 && white.size()==0) {
		 return true;
	 }
	 return false;
 }

 vector<pair<string, unsigned long long int>> Board::getChildren(char c){
	 setColor(c);
	 
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
						 if (isLegalMove(x1, y1, x2, y2)) {
							 string c_move = "m" + ReconstructMove(x1, y1, x2, y2);
							 if (HH.find(c_move) == HH.end()) { HH.emplace(c_move, 0); }
							 children.emplace_back(make_pair(c_move, HH[c_move]));
						 }
					 }
				 }
			 }
		 }
	 }
	  //cout << "\nchildren of current player: " <<endl; //testing children
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

 bool Board::FindNeighbors(int x, int y,int to_x, int to_y)
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

 string Board::ReconstructMove(int x1, int y1, int x2, int y2){
	 //ex:move 0302 = d3c4 since rows are in decending order
	 string xx1 = findRow(n-x1-1);
	 string xx2 = findRow(n-x2-1);
	 return xx1 + std::to_string(y1+1) + xx2 + std::to_string(y2+1);
 }
 void Board::play(unsigned int& fb, unsigned int& fw, unordered_multiset<string>& Mb, unordered_multiset<string>& Mw, char color,string move, array<string, 64>& board, string& JumpsAllowed, stack<array<string, 64>>& history) {
	 
	 setColor(color);

	 vector<tuple<int, int, int, int>> blob_moves;
	 if (game_over == true) {
		 cout << "\n[game is finished, no further moves accepted]" << endl;
		 return;
	 }
	 /*load board and check if we have a board
	 promt user to make a board if none*/
	 if (board.empty()) {
		 cout << "Please set up your board first" << endl;
		 return;
	 }

	 history.push(board);//save move for first redo

	 blob_moves = getLegalMoves(color);
	 if (blob_moves.size() == 0) { // pass player if he has no legal moves left
		 string pass_move = "ms1s2";
		 setMove(pass_move);//set pass move
		 return;
	 }
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
	
	bool right_format=false;
	if (move[0] == 'm'){
		right_format = true;
	}
	bool legal = isLegalMove(x1, y1, x2, y2);
	bool isClone = isCloneMove(x1, y1, x2, y2);
	if (legal && right_format) {//check type of move (duplicate/jump) //keep count of jumps

		if (isClone) {
			//cout << "\n[its a clone move ]" << endl;
			makeMove(isClone, x1, y1, x2, y2, board);
		}
		else if (!isClone) {
			//cout << "\n[its a jump move ]" << endl;
			makeMove(isClone, x1, y1, x2, y2, board);
			JumpsAllowed.append("j");//append "j" to jumpsAllowed string to keep track on jumps made
		}
		history.push(board); //save history after the move
	}
	else {
		cout << "\n[illegal move !]" << endl;
	}
 }

 bool Board::gameOver(){
	 int black_score = getScore('b');
	 int white_score = getScore('w');
	 string maxJumps;
	 maxJumps.insert(0, 50, 'j');

	 cout << "\nSCORE: " << black_score << " (b) " << white_score << " (w)" << endl;
	 // check if both b & w has no legal moves
	 bool no_moves;
	 no_moves = nolegalMovesLeft();
	 //check for 3 repetition rule for black and white
	 bool repetition_occured = (fb == 4 || fw == 4) ? true : false; //must =4 cuz the function count the move as well 

	 /*cout << "\nno moves: " << no_moves << endl;
	 cout << "fb fw: " << fb << fw << endl;
	 bool end = JumpsAllowed.find(maxJumps) != string::npos;
	 cout << "string jump: " << end << endl;*/

	 //check if game is over and print result (4. game also ends if a player lost all his pieces)
	 if (JumpsAllowed.find(maxJumps) != string::npos || no_moves == true || repetition_occured == true || (black_score==0||white_score==0))
	 {
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
	 for the 3 rule repetetition, found must be = 4 not 3
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
 int Board::indexOf(char color) {
	 if (color == 'b') { return 1; }
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
 bool Board::SortChild(const pair<string, unsigned long long int> &a, const pair<string, unsigned long long int> &b) {
	 return a.second > b.second;
 }
 int Board::AlphaBeta(int depth,char color, array<string, 64>& board, int alpha, int beta, stack<array<string, 64>>& history){
	 //code for AlphaBeta & negascout is referenced from the course notes + references in README.txt
	//naming as per a1.txt

	 record.calls++; //get no. of function calls
	 setColor(color);//make sure color is correct for interial functions
	 
	 if (depth == 0 || game_over == true) { return getValue(); }//when game is over a node was terminal

	 unsigned long long int child_hash = getHash();

	 TTEntry& e = checkTT(child_hash, depth, alpha, beta);
	 if (alpha >= beta) { record.TTC++; return e.value; }

	 vector<pair<string, unsigned long long int>> children = getChildren(color);//all children of current player

	 if (children.size() == 0) { return getValue(); }

	 string temp_best_move = "ms1s2";
	 int move_idx = -1;
	 int start_idx = -1;
	 int start_score = -100;
	 //char opopent_color = (color == 'b') ? 'w' : 'b';

	 //find the best move from the table
	 if (e.hash == child_hash) {
		 for (unsigned int i = 0; i < children.size(); i++) {
			 if (children[i].first == e.bestMove) {
				 temp_best_move = e.bestMove;
				 move_idx = i;
				 children[i].second = 0;
				 //cout << "\nmove idx: " << move_idx << endl; //test
				 break;
			 }
		 }
	 }
	 //find best move with depth 1 search
	 for (unsigned int i = 0; i < children.size() && children[i].first != temp_best_move; i++) {
		
		 play(fb, fw, Mb, Mw, color,children[i].first, board, JumpsAllowed, history);
		
		 int v = -getValue();

		 if (start_score < v) {
			 start_score = v;
			 start_idx = i;
		 }
		 //undo bad move
		 undoMove(history, board);
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
			 //get best move
			 play(fb, fw, Mb, Mw, color,children[move_idx].first,board, JumpsAllowed, history);
			 temp_hash = getHash();
			 // assumes alternating moves
			 value = -AlphaBeta(depth - 1,color, board, -beta, -new_alpha, history);

			 //undo bad move
			 undoMove(history, board);

			 if (value > score) {
				 child_best_move = children[move_idx].first;
				 best_hash = temp_hash;
				 score = value;

				 if (score > new_alpha) { new_alpha = score; }
				 if (score >= beta){ 
					 record.Cut_BF++;
					 record.successors += 1;//when a cut occurs its a leaf node (used for CBF average)
					 cut = true; 
					 break; }
			 }
		 }
		 // get best value move
		 if (start_idx != -1) {

			 play(fb, fw, Mb, Mw, color,children[start_idx].first,board, JumpsAllowed, history);
			 
			 value = -AlphaBeta(depth - 1,color, board, -beta, -new_alpha, history);
			 temp_hash = getHash();

			 //undo
			 undoMove(history, board);

			 if (value > score) {
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
			 play(fb, fw, Mb, Mw, color,children[i].first,board, JumpsAllowed, history);
			 value = -AlphaBeta(depth - 1, color, board, -beta, -new_alpha, history);
			 temp_hash = getHash();

			 //undo
			 undoMove(history, board);
			
			 if (value > score) {
				 child_best_move = children[i].first;
				 best_hash = temp_hash;
				 score = value;
				 if (score > new_alpha) { new_alpha = score; }
				 if (score >= beta) { 
					 record.Cut_BF++; 
					 record.successors += 1 + i;//+i we get the sucessor for each iteration
					 cut = true; 
					 break; }
			 }
		 }
		 break;
	 }
	 if (cut == true) { HH[child_best_move] += 2 ^ depth; }
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
		 play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
		 int v = -getValue();
		 if (start_score < v) {
			 start_score = v;
			 start_idx = i;
		 }
		 //undo bad move
		 undoMove(history, board);
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
			 play(fb, fw, Mb, Mw, color, children[move_idx].first, board, JumpsAllowed, history);
			 value = -NegaScout(depth - 1, color, board, -beta, -new_alpha, history);
			 temp_hash = getHash();
			 undoMove(history, board);
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
			 play(fb, fw, Mb, Mw, color, children[start_idx].first, board, JumpsAllowed, history);
			 value = -NegaScout(depth - 1, color, board, -beta, -new_alpha, history);
			 temp_hash = getHash();
			 undoMove(history, board);
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

		 play(fb, fw, Mb, Mw, color, children[0].first, board, JumpsAllowed, history);
		 value = -NegaScout(depth - 1, color, board, -beta, -new_alpha, history);
		 temp_hash = getHash();
		 undoMove(history, board);
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
				 play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
				 int s = -NegaScout(depth - 1, color, board, -upper_bound, -lower_bound, history);
				 temp_hash = getHash();
				 undoMove(history, board);
				 if (s >= upper_bound && s < beta) {
					 play(fb, fw, Mb, Mw, color, children[i].first, board, JumpsAllowed, history);
					 s = -NegaScout(depth - 1, color, board, -beta, -s, history);
					 temp_hash = getHash();
					 undoMove(history, board);
				 }
				 if (s > score) {
					 best_hash = temp_hash;
					 child_best_move = children[i].first;
					 score = s;
				 }
				 if(score >= beta) {
					 cut = true; 
					 break;
					 record.Cut_BF++;
					 record.successors += 1+i;//+i we get the sucessor for each iteration
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

	 int score=0;
	 unsigned long long int bestHash;
	 string best_move;

	 //initialize search parameters at the beginig of every new search
	 record.calls = 0;
	 record.Cut_BF = 1;
	 record.successors = 0;
	 record.TTC = 0;
	 record.TTF = 0;
	 record.TTQ = 0;
	 
	 for (int i = 1; i < d; i++) {
		 // during search AI playes two moves(2 players) and print result and borad
		 if (search_mode == 1) {
			 cout << "\n[Mode 1: AlphaBeta search...]" << endl;
			 score = AlphaBeta(depth, color, board, -100, 100, history); 
		 }
		 else if (search_mode == 2) {
			 cout << "\n[Mode 2: NegaScout search...]" << endl;
			 score = NegaScout(depth, color, board, -100, 100, history);
		 }
		 std::chrono::duration<double> time = std::chrono::system_clock::now() - startTime;

		 if (time_over(startTime)) {
			 search_results(time, score, i);
			 break;
		 }
		 else {
			 bestHash = getHash();
			 best_move = TT[bestHash % 256000].bestMove;
			 cout << "\nmove: " << best_move << endl;

		 }
		 search_results(time, score, i);
	 }
	 cout << "\nfinal board:" << endl;
	 PrintBoard();
	 cout << "\nsearch score: " << score << endl;
 }
 //print result of search
 void Board::search_results(std::chrono::duration<double> time, int _score, int i) {
	 //time: total time used so far for all iterations in seconds
	 //speed:# of Alpha-Beta calls / total time so far
	 //Cut - BF: The average number of successors considered in states in which a cut - off occurs.Count only nodes where at least one move
	 //i: number of times

	 record.CTM = (color == 'b') ? "##" : "()";
	 //--------------------------HEADER
	 cout << left << setw(9) << setfill(' ') << "CTM";
	 cout << left << setw(9) << setfill(' ') << "D";
	 cout << left << setw(9) << setfill(' ') << "Time";
	 cout << left << setw(9) << setfill(' ') << "Calls";
	 cout << left << setw(9) << setfill(' ') << "Speed";
	 cout << left << setw(9) << setfill(' ') << "TTQ";
	 cout << left << setw(9) << setfill(' ') << "TTF";
	 cout << left << setw(9) << setfill(' ') << "TTC";
	 cout << left << setw(9) << setfill(' ') << "CBF";
	 cout << left << setw(9) << setfill(' ') << "Value" << endl;

	 //------------------------PRINT VALUES
	 cout << left << setw(9) << setfill(' ') << record.CTM;
	 cout << left << setw(9) << setfill(' ') << i;
	 cout << left << setw(12) << setfill(' ') << time.count() ;
	 cout << left << setw(12) << setfill(' ') << record.calls;
	 cout << left << setw(12) << setfill(' ') << record.calls/time.count();
	 cout << left << setw(12) << setfill(' ') << record.TTQ;
	 cout << left << setw(12) << setfill(' ') << record.TTF;
	 cout << left << setw(12) << setfill(' ') << record.TTC;
	 cout << left << setw(12) << setfill(' ') << record.successors / record.Cut_BF;
	 cout << left << setw(12) << setfill(' ') << _score << endl;

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


	 cout << "[initializing board of size " << n << "]" << endl;
	 //display default board upon initilization for testing
	 DisplayBoard();
	 //initialize the table
	 initTable(n);

	 string cmd;
	 char player1, player2;

MENUE:
	 while (CMD != "q") {
		 cout << "\nEnter cmd: ";
		 getline(cin, cmd);
		 setCMD(cmd);

		 if (CMD[0] == 's') {
			 //need to clear history if s is interred
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
			 player1 = 'b';
			 player2 = 'w';
			 if (search_mode == 1) { cout << "[start search with player " << color << "]" << endl; }
			 else { cout << "\n[Black is to move ]"; }
			 continue;
		 }
		 else if (CMD[0] == 'w') {
			 current_turn = 1;
			 
			 player1 = 'w';
			 player2 = 'b';
			 setColor('w');
			 if (search_mode == 1) { cout << "[start search with player " << color << "]" << endl; }
			 else { cout << "\n[white is to move ]"; }
			 continue;
		 }
		 else if (CMD[0] == 'u') {
			 if (history.empty()) {
				 cout << "[No previous move available]" << endl;
			 }
			 else {
				 cout << "\nUndoing your latest move.. " << endl;
				 history.pop();
				 undoMove(history, board);
				 PrintBoard();
			 }
			 continue;
		 }
		 else if (CMD[0] == '1') {
			 setMode(1);
			 cout << "\nGetting Mode 1 settings... " << endl;
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
			 cout << "\nGetting Mode 2 settings: " << endl;
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
		 else if (CMD[0] == 'm' && CMD.size() == 5 && search_mode == 0) {
			 //setMode(0); //no search: user is playing
			 setMove(CMD);
			 play(fb, fw, Mb, Mw,color, move, board, JumpsAllowed, history);
			 gameOver();
			 PrintBoard();
		 }
		 else if (CMD[0] == 'q') { cout << "\nQuitting Game.. " << endl; exit(0); }

		 else if (CMD[0] == 'g') { 
			 goto SEARCH; 
		 }//start search

		 else { cout << "\nInvalid Command " << endl; }
	 }


 SEARCH:
	 //search start at this tag:
	 //start search, print results, player switch, go back to get user input again

	 //check if we need ft or rt for prenting startimg info
	 if (TimeFlag == 2) {
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
			 cout << "Search over ..." << endl;
			 gameOver();
			 return;
		 }
	 }
	 std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
	 
	 //start search (iterative depening)
	 Search(depth, -100, 100, startTime);

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
	 switchPlayer();
	 gameOver();
	 setMode(0);
	 goto MENUE;

 }

