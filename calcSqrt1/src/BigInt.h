#pragma once
#include "stdafx.h"
#define BIGINT_SIZE 32
typedef unsigned int chunk;
class BigInt
{
	std::vector<chunk> data;

	void carry(const int index);
	char intToHex(unsigned char input) const;
public:
	BigInt(void) { }
	BigInt(chunk assignment) {
		data.push_back(assignment);
	}
	BigInt(double assign, int prec = 2) {
		for (int i = 0; i < prec; i++) {
			chunk t = (chunk)(floor(assign));
			data.push_back(t);
			assign -= t;
			assign *= pow(2.0, BIGINT_SIZE);
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
		return data.size() * BIGINT_SIZE / 4;
	}
};

