// Ataax.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>

#include "Board.h"

int main() {
	cout.setf(ios_base::unitbuf);

	cout << "\n\n\t\t================[ ATAXX - 2 Players ]================" << endl;
	cout << endl;

	Board b;
	b.Game();

	return 0;
}
