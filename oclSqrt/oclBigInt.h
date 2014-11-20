#pragma once
#include "stdafx.h"
//#include "BigInt.h"
// assumes limb size to be 32

class BigInt;

class oclBigInt
{
	static const size_t workItemsPerGroup = 512;

public:
	// size in 32-bits
	static void fillBuffer(cl_mem buffer, unsigned int pattern, unsigned int offset, unsigned int size);
	static size_t getNumWorkItems(size_t numLimbs);

	cl_mem limbs;
	size_t numLimbs;

	void resize(const size_t size);
	void rmask(const oclBigInt &mask);
	void truncate();

public:
	static cl_context context;
	static cl_command_queue queue;
	static cl_kernel fillKernel;
	static cl_kernel shlKernel;
	static cl_kernel shrKernel;
	static cl_kernel addKernel;
	static cl_kernel carryKernel;
	static cl_kernel negKernel;
	static cl_kernel mulKernel;
	static cl_kernel rmaskKernel;
	static cl_kernel countKernel;

	oclBigInt(void);
	oclBigInt(unsigned int i, unsigned int pos = 0U);
	oclBigInt(double d, unsigned int prec = 2U);
	oclBigInt(std::vector<unsigned int> &i);
	//oclBigInt(const BigInt &i);
	~oclBigInt(void);

	bool operator>(const int n);
	oclBigInt &operator<<=(const int d);
	oclBigInt &operator>>=(const int d);
	oclBigInt &operator+=(const oclBigInt &n);
	oclBigInt &operator-=(const oclBigInt &n);
	oclBigInt &operator*=(const oclBigInt &n);

	void setNeg();
	void copy(oclBigInt &n) const;
	BigInt toBigInt() const;
	void verify();
	size_t getNumLimbs() const { return numLimbs; }

	friend std::ostream& operator<<(std::ostream &os, const oclBigInt &n);
};

