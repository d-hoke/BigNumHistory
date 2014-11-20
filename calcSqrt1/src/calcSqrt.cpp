// calcSqrt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BigInt.h"

void wait(int sec) {
	time_t sT = time(0);
	while (time(0) - sT < sec) {
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	using std::endl; using std::cout;

	//BigInt x = 0.7;
	//BigInt add = (chunk)3;
	//add = add >> 1;
	//for (int i = 0; i < 6; i++) { // order is not flexible
	//	BigInt t = x;
	//	cout << "t = x  : " << t << endl;
	//	t = t * x;
	//	cout << "t *= x : " << t << endl; 
	//	t = ~t;
	//	cout << "t = ~t : " << t << endl;
	//	t += add;
	//	cout << "t += add : " << t << endl;
	//	x = x * t;
	//	cout << "x *= t : " << x << endl;
	//	//x = x * (~(x * x) + add);

	//	//cout << "x = " << x << endl;
	//	if (i > 1) x.prune();
	//}

	//x.prune();
	//x <<= 1;
	//cout << "sqrt(2) = " << x << endl;

	srand(time(0));

	const size_t sizeX = 0x200;
	BigInt x(0U, sizeX - 1);
	for (int i = 0; i < sizeX; i++) {
		unsigned int t = rand() << 16 | rand();
		x += BigInt(t, i);
	}

	cout << "x is " << x.numLimbs() << " limbs." << endl;

	DWORD sT;
	BigInt t;
	double dT;
	int runs;
	for (int runs = 1; runs < 1000000000; runs += runs) {
		sT = timeGetTime();
		for (int i = 0; i < runs; i++) {
			t = x + x;
		}
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "t = x + x (" << runs << " runs) \t: " << dT << "s" <<  endl;

		sT = timeGetTime();
		for (int i = 0; i < runs; i++) {
			t = x * x;
		}
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "t = x * x (" << runs << " runs) \t: " << dT << "s" <<  endl;

		sT = timeGetTime();
		for (int i = 0; i < runs; i++) {
			t = x;
			t.squared(0x1000);
		}
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "t = x.squared() (" << runs << " runs) \t: " << dT << "s" <<  endl;
	}
	//sT = timeGetTime();
	//for (runs = 0; timeGetTime() - sT < 1000; runs++) {
	//	t = x * x;
	//}
	//dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
	//cout << "t = x * x \t: " << dT << "s" <<  endl;
	//for (int i = 2; i < 1024; i++) {
	//	sT = timeGetTime();
	//	//for (int i = 0; i < runs; i++) {
	//	for (runs = 0; timeGetTime() - sT < 1000; runs++) {
	//		t = x.squared(i);
	//	}
	//	dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
	//	cout << "t = x ^ 2 (" << runs << " runs, " << i << " min size) \t: " << dT << "s" <<  endl;

	//	//cout << endl;
	//}

	// S = 2
	//const int runs = 1;
	//double dT = 0.0;
	//int digits;
	//for (int a = 0; a < runs; a++) {
	//	std::cout << "run " << a << std::endl;
	//	int sT = timeGetTime();
	//	BigInt x = 0.7;
	//	BigInt add = (chunk)3;
	//	add = add >> 1;
	//	for (int i = 0; i < 4; i++) { // order is not flexible
	//		//std::cout << "iteration " << i << std::endl;
	//		//cout << "x * x = " << (x * (~(x*x) + add)).get() << endl;
	//		x = x * (~(x.squared()) + add);
	//		//cout << "x^2 = " << x.get() << endl;
	//		//y = (x * x) << 1;
	//		//x = (x >> 3) * (~(y * (~(y * 3) + 10)) + 15);
	//		
	//		if (i > 1) {
	//			//std::cout << "x has " << x.numDigits() << " digits." << std::endl;
	//			x.prune();
	//			//std::cout << "x has " << x.numDigits() << " digits." << std::endl;
	//			//std::cout << "x = " << x.get() << std::endl;
	//		} else {
	//			//cout << "x = " << x.get() << endl;
	//		}
	//		//cout << endl;
	//	}
	//	x = x << 1; // *= S

	//	std::cout << "square root of 2 is " << x.get() << "\nneg square root of 2 is " << (~x).get() << std::endl;
	//	dT += (double)(timeGetTime() - sT) / 1000.0;
	//	digits = x.numDigits();
	//}

	//dT /= runs;
	//double dps = digits / dT;

	//std::cout << digits << " digits, " << dT << " seconds, " << dps << " digits/sec" << std::endl;

	cout << "waiting for input to quit." << endl;
	std::cin.get();
	
	//std::cout << "pausing for 5 seconds";
	//for (int i = 0; i < 10; i++) {
	//	wait(1);
	//	std::cout << ".";
	//}
	return 0;
}

