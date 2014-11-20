#pragma once
#include "stdafx.h"
//#include "BigInt.h"
// assumes limb size to be 32
//#define DEBUG_SHIFTMUL
#define DEBUG_CL
//#define DEBUG_VERIFY

class BigInt;

class oclBigInt
{
	static const size_t workItemsPerGroup = 512;
	static bool profile;

public:
	static void safeCL(const cl_int error, const char *call, const char *action);
	static void initCheckCL(cl_kernel &kernel, const cl_int argOffset, const char *funcName,
		cl_mem &error_d, cl_mem &ixError_d, cl_uint &szError);
	static void checkCL(const char *funcName, const char *kernelName,
		cl_mem &error_d, cl_mem &ixError_d, cl_uint &szError);
	// size in 32-bits
	static void fillBuffer(cl_mem buffer, unsigned int pattern, unsigned int offset, unsigned int size);
	static size_t getNumWorkItems(size_t numLimbs);

	cl_mem limbs;
	cl_mem carry_d;
	size_t numLimbs;

	//static void carry(cl_mem &limbs, cl_mem &carry_d, cl_mem &n2_d, int minWidth);
	static void addCarry(cl_mem &buffer, cl_mem &carry_d, int minWidth, cl_ulong &timer);
	static void shortAddOff(cl_mem &n, cl_mem &n2, int szN2, cl_mem &needAdd, int offset, cl_ulong &timer);
	void setLimbs(cl_mem newLimbs, cl_mem newCarry, size_t newSize);
	void rmask(const oclBigInt &mask);
	void truncate();
	//oclBigInt &oldMul(const oclBigInt &n);
	oclBigInt &baseMul(const oclBigInt &n);
	void split(const oclBigInt &n, oclBigInt &n_1, oclBigInt &n_0, int p);
	oclBigInt &shiftMul(oclBigInt &n, int minSize);
	void toggle(cl_mem dCarry);

public:
	static cl_context context;
	static cl_command_queue queue;
	static cl_kernel fillKernel;
	static cl_kernel shlKernel;
	static cl_kernel shrKernel;
	static cl_kernel addKernel;
	static cl_kernel carryKernel;
	static cl_kernel negKernel;
	//static cl_kernel mulKernel;
	static cl_kernel rmaskKernel;
	static cl_kernel countKernel;
	static cl_kernel mul2Kernel;
	//static cl_kernel carry2Kernel;
	//static cl_kernel oldMulKernel;
	static cl_kernel carryOneKernel;
	static cl_kernel shortAddOffKernel;
	static cl_kernel checkAddKernel;
	static cl_kernel toggleKernel;
	static cl_ulong time_baseMul;
	static cl_ulong time_carry;
	static cl_ulong time_carry2;
	static cl_ulong time_fill;
	static cl_ulong time_shr;
	static cl_ulong time_shl;
	static cl_ulong time_count;
	static cl_ulong time_add;
	static cl_ulong time_neg;
	static cl_ulong time_carryOne;
	static cl_mem needCarry_d;

	static void init();
	static void close();
	static void resetProfiling();
	static void printProfiling(int runs = 1, double bigTotal = 1.0);

	oclBigInt(void);
	oclBigInt(size_t size) ;
	oclBigInt(int i, unsigned int pos = 0U);
	oclBigInt(double d, unsigned int prec = 2U);
	oclBigInt(BigInt &n);
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
	oclBigInt &mul2(const oclBigInt &n, int minSize = 0x8000);
	void resize(const size_t size);
	void move(oclBigInt &n);

	friend std::ostream& operator<<(std::ostream &os, const oclBigInt &n);
};

