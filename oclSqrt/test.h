#ifndef TEST_H
#define TEST_H

#include "stdafx.h"
#include "oclBigInt.h"

void test();

void test_oclAdd();
BigInt cpuAdd(BigInt &x, BigInt &y);
BigInt oclAdd(BigInt &x, BigInt &y);
void oclAddOp(oclBigInt &x, oclBigInt &y);

void test_oclMul();
BigInt cpuMul(BigInt &x, BigInt &y);
BigInt oclMul(BigInt &x, BigInt &y);
void oclMulOp(oclBigInt &x, oclBigInt &y);

void test_newBaseMul();
BigInt cpuBaseMul(BigInt &x, BigInt &y);
BigInt cpuBaseMul2(BigInt &x, BigInt &y);

void testIterateFRand(BigInt (*opA)(BigInt&, BigInt&), BigInt (*opB)(BigInt&, BigInt&),
	std::string type, int sz_min, int sz_max);
void testIterateEvenUneven(BigInt (*opA)(BigInt&, BigInt&), BigInt (*opB)(BigInt&, BigInt&),
	uint32_t (*getLimbX)(), uint32_t (*getLimbY)(), std::string type,
	int sz_min, int sz_max);
void testIterate(BigInt (*opA)(BigInt&, BigInt&), BigInt (*opB)(BigInt&, BigInt&),
	uint32_t (*getLimbX)(), uint32_t (*getLimbY)(), size_t sz_min, size_t sz_max,
	void (*incX)(size_t&), void (*incY)(size_t&));
uint32_t getF();
uint32_t getRand();
void doubleVar(size_t &var);
void identity(size_t &var);
BigInt oclOp(void (*op)(oclBigInt&, oclBigInt&), BigInt &x, BigInt &y);

#endif