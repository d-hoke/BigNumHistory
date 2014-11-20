#pragma once
#include "stdafx.h"
#include "oclBigInt.h"

#define BIGINT_LIMB_SIZE 32
typedef unsigned int limb;
class BigInt
{
	friend class oclBigInt;

public:

	std::vector<limb> limbs;

	void carry(const int index);
	void addCarry(BigInt carry);
	void rmask(const BigInt &mask);
	char intToHex(unsigned char input) const { return input + (input > 9 ? 55 : 48); }
	BigInt oldMul(const BigInt &n) const;
	BigInt baseMul(const BigInt &n) const;
	BigInt baseMul2(const BigInt &n) const;
	BigInt shiftMul(const BigInt &n, int minSize) const;

public:
	BigInt(void);
	BigInt(unsigned int i, unsigned int pos = 0);
	BigInt(double d, int prec = 2);
	BigInt(const cl_mem buffer, const size_t numLimbs, const cl_command_queue queue);
	~BigInt(void);

	void set(unsigned int i, unsigned int pos);
	void fill(unsigned int pattern, int ix_min, int ix_max);

	BigInt operator<<(const int d) const;
	BigInt &operator<<=(const int d);
	BigInt operator>>(const int d) const;
	BigInt &operator>>=(const int d);
	BigInt operator+(const BigInt &n) const;
	BigInt &operator+=(const BigInt &n);
	BigInt operator-(const BigInt &n) const;
	BigInt &operator-=(const BigInt &n);
	BigInt operator*(const BigInt &n) const;
	BigInt mulDigit(const BigInt &n, int minSize = 0x20) const;
	//BigInt &operator*=(const BigInt &n);
	bool operator==(const BigInt &n) const;
	bool operator>(const unsigned int other) const;

	BigInt neg() const;
	BigInt &setNeg();

	void verify();
	std::string get() const;
	size_t numLimbs() const { return limbs.size(); }
	//oclBigInt toOcl();
	void truncate();
	void out(const char *fileName);
	void in(const char *fileName);

	friend std::ostream& operator<<(std::ostream &os, const BigInt &n);
};

