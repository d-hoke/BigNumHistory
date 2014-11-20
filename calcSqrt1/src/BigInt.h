#pragma once
#include "stdafx.h"
#define BIGINT_SIZE 32
typedef unsigned int chunk;
class BigInt
{
	std::vector<chunk> data;

	void carry(const int index);
	char intToHex(unsigned char input) const;
	void truncate();
	BigInt getTruncate() const;
	void rmask(const BigInt &mask);
public:
	BigInt(void) { }
	BigInt(chunk assignment, int position = 0) {
		while (data.size() <= position) {
			data.push_back(0);
		}
		data[position] = assignment;
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
	BigInt operator-(const BigInt &n) const;
	BigInt &operator-=(const BigInt &n);
	BigInt operator*(const BigInt &_other) const;
	BigInt operator*(const int other) const;
	BigInt operator~() const;
	bool operator>(const unsigned int other) const {
		if (data[0] > other || (data[0] == other && data.size() > 1)) {
			return true;
		}
		return false;
	}
	BigInt squared(const int minSize) const;
	void split(BigInt &a, BigInt &b) const;
	void prune();
	std::string get() const;
	int numDigits() const {
		return data.size() * BIGINT_SIZE / 4;
	}
	size_t numLimbs() const { return data.size(); }

	friend std::ostream& operator<<(std::ostream &os, const BigInt &n);
};

