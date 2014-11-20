// calcSqrt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BigInt.h"

void wait(int sec) {
	time_t sT = time(0);
	while (time(0) - sT < sec) {
	}
}

void prune(BigInt &x) {
	BigInt e = x * x;
	e <<= 1;
	e = ~e;
	e = e + 1;
	x.rmask(e);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//std::cout << "BigInt:" << std::endl;
	// S = 2
	double dT = 0.0;
	int digits;
	for (int a = 0; a < 5; a++) {
		int sT = timeGetTime();
		BigInt x = 0.7;
		for (int i = 0; i < 9; i++) {
			//std::cout << "iteration " << i << std::endl;
			//std::cout << "x = " << x.get() << std::endl;
			BigInt changeX = x * x;
			//std::cout << "x * x = " << changeX.get() << std::endl;
			changeX <<= 1; // *= 2
			//std::cout << "x * x * 2 = " << changeX.get() << std::endl;
			changeX = ~changeX;
			changeX = changeX + 3;
			//std::cout << "3.0 - x * x * 2 = " << changeX.get() << std::endl; 
			x = x >> 1;
			//std::cout << "x / 2 = " << x.get() << std::endl;
			x = x * changeX;
			//std::cout << "x / 2 * (3.0 - x * x * 2) = " << x.get() << std::endl << std::endl;

			if (i % 2 == 1) {
				prune(x);
				//std::cout << "x = " << x.get() << std::endl;
			}
		}

		prune(x);

		x = x << 1; // *= S

		std::cout << "square root of 2 is " << x.get() << std::endl;
		dT += (double)(timeGetTime() - sT) / 1000.0;
		digits = x.numDigits();
	}

	dT /= 5.0;
	double dps = digits / dT;

	std::cout << digits << " digits, " << dT << " seconds, " << dps << " digits/sec" << std::endl;

	//std::cin.get();
	
	//std::cout << "pausing for 5 seconds";
	//for (int i = 0; i < 10; i++) {
	//	wait(1);
	//	std::cout << ".";
	//}
	return 0;
}

