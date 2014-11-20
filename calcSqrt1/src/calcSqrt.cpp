//============================================================================
// Name        : calcSqrt.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <cstdio>
#include <iostream>
#include "InfBinFraction.h"

double sqrtBabylon(double _num, double _apx, int _iter);
double sqrtNewton(double _num, double _apx, int _iter);

double notation(double _val) {
	return _val - floor(_val / 10.0) * 10.0;
}

int main() {
//	printf("babylon iterations - ");
//	for (int i = 1; i < 7; i++) {
//		printf("%d:%f ", i, sqrtBabylon(2.0, 1.0, i));
//	}
//	printf("\nnewton iterations - ");
//	for (int i = 1; i < 7; i++) {
//		printf("%d:%f ", i, sqrtNewton(2.0, 1.0, i));
//	}

	InfBinFraction myFrac = InfBinFraction(3.516);
	InfBinFraction frac2(5.2);
	myFrac = myFrac + frac2;

	std::cout << myFrac;
	return 0;
}

double sqrtBabylon(double _num, double _apx, int _iter) {
	for (int i = 0; i < _iter; i++) {
		_apx = (_apx + _num / _apx) / 2;
	}
	return _apx;
}

double sqrtNewton(double _num, double _apx, int _iter) {
	for (int i = 0; i < _iter; i++) {
		_apx = (_apx * (3 - _num * _apx * _apx)) / 2;
	}
	return _apx * _num;
}
