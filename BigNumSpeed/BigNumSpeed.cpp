// BigNumSpeed.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Deque.h"

void CinIgnoreLine()
{
	using std::cin; using std::numeric_limits; using std::streamsize;

	cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

template <class InputType>
void GetInput(InputType &input, const std::string request = ">", const std::string badInput = "Bad input, try again.")
{
	using std::cout; using std::endl; using std::cin;

	while (true) {
		cout << request << endl;
		if (cin >> input) {
			break;
		} else {
			cout << badInput << endl;
			cin.clear();
			CinIgnoreLine();
		}
	}
	CinIgnoreLine();
}


int _tmain(int argc, _TCHAR* argv[])
{
	using std::cout; using std::endl;

	Deque<int> myDeque;

	for (int i = 0; i < 15; i++) {
		myDeque.PushBack(i);
		cout << myDeque.ToString() << endl;
	}

	for (int i = 0; i < 5; i++) {
		myDeque.PopBack();
		cout << myDeque.ToString() << endl;
	}

	std::cin.get();

	return 0;
}

