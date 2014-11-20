/*
 * InfBinFraction.h
 *
 *  Created on: Nov 11, 2011
 *      Author: peter
 */

#ifndef INFBINFRACTION_H_
#define INFBINFRACTION_H_

#include <list>
#include <cmath>
#include <iostream>
#include <cstdio>

#define MAX_VAL 0xFFFFFFFF

struct digit {
	unsigned long long data;
	int place;

	digit() : data(0), place(0) { };
	digit(unsigned long long _data) : data(_data), place(0) { };
	digit(unsigned long long _data, int _place) : data(_data), place(_place) { };
};

typedef std::list<digit> lst;

class InfBinFraction {
	void shiftPlace(lst::iterator _fullDigit);

	bool isPositive;
	std::list<digit> digits;
public:
	InfBinFraction() : digits(1, digit()) { };
	InfBinFraction(unsigned long long _val) : digits(1, digit(_val)) { };
	InfBinFraction(double _val) { unsigned long long whole = floor(_val); digits.push_back(digit(whole, 0));
	digits.push_back(digit((_val - whole) * MAX_VAL, -1)); };
	virtual ~InfBinFraction();

	InfBinFraction operator+(InfBinFraction &_rhs);
	InfBinFraction operator-(InfBinFraction &_rhs);

	friend std::ostream& operator<<(std::ostream& _stream, InfBinFraction _val);
};

#endif /* INFBINFRACTION_H_ */
