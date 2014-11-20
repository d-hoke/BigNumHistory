// BigNumApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BigNum.h"

int _tmain(int argc, _TCHAR* argv[])
{
	BigNum lhs = 2;
	lhs *= BigNum(2);
	std::cout << "My test. 2*2=" << lhs.getStr() << std::endl;
	std::cin.get();
	return 0;
}

