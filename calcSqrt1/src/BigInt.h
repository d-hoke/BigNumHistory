#pragma once
#include "stdafx.h"
class BigInt
{
	std::vector<unsigned char> data;

	void carry(const int index);
	char intToHex(unsigned char input) const;
public:
	BigInt(void) { }
	BigInt(unsigned char assignment) {
		data.push_back(assignment);
	}
	BigInt(double assign, int prec = 2) {
		for (int i = 0; i < prec; i++) {
			int t = (int)(floor(assign));
			data.push_back(t);
			assign -= t;
			assign *= 256.0;
		}
	}
	~BigInt(void);

	BigInt operator<<(const int degree) const;
	BigInt &operator<<=(const int degree);
	BigInt operator>>(const int degree) const;
	BigInt &operator>>=(const int degree);
	BigInt operator+(const BigInt &_other) const;
	BigInt operator+(const int other) const;
	BigInt &operator+=(const BigInt &_other);
	BigInt operator*(const BigInt &_other) const;
	BigInt operator~() const;
	void rmask(const BigInt &mask);
	std::string get() const;
	int numDigits() const {
		return data.size() * 2;
	}
};

