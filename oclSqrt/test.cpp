#include "stdafx.h"
#include "test.h"

void test() {
	using std::cout; using std::endl;
	srand((unsigned int)time(0));

	test_oclAdd();
	test_oclMul();
	test_newBaseMul();
}

void test_oclAdd() {
	testIterateFRand(cpuAdd, oclAdd, "add", 2, 0x10000);
}

BigInt cpuAdd(BigInt &x, BigInt &y) {
	return x + y;
}

BigInt oclAdd(BigInt &x, BigInt &y) {
	return oclOp(oclAddOp, x, y);
}

void oclAddOp(oclBigInt &x, oclBigInt &y) {
	x += y;
}

void test_oclMul() {
	testIterateFRand(cpuMul, oclMul, "mul", 2, 0x800);
}

BigInt cpuMul(BigInt &x, BigInt &y) {
	return x * y;
}

BigInt oclMul(BigInt &x, BigInt &y) {
	return oclOp(oclMulOp, x, y);
}

void oclMulOp(oclBigInt &x, oclBigInt &y) {
	x *= y;
}

void test_newBaseMul() {
	testIterateFRand(cpuBaseMul, cpuBaseMul2, "newBaseMul", 1, 0x800);
}

BigInt cpuBaseMul(BigInt &x, BigInt &y) {
	return x.baseMul(y);
}

BigInt cpuBaseMul2(BigInt &x, BigInt &y) {
	return x.baseMul2(y);
}

void testIterateFRand(BigInt (*opA)(BigInt&, BigInt&), BigInt (*opB)(BigInt&, BigInt&),
	std::string type, int sz_min, int sz_max) {
		testIterateEvenUneven(opA, opB, getF, getF, type + " : f, f", sz_min, sz_max);
		testIterateEvenUneven(opA, opB, getF, getRand, type + " : f, rand", sz_min, sz_max);
		testIterateEvenUneven(opA, opB, getRand, getF, type + " : rand, f", sz_min, sz_max);
		testIterateEvenUneven(opA, opB, getRand, getRand, type + " : rand, rand", sz_min, sz_max);
		std::cout << std::endl;
}

void testIterateEvenUneven(BigInt (*opA)(BigInt&, BigInt&), BigInt (*opB)(BigInt&, BigInt&),
	uint32_t (*getLimbX)(), uint32_t (*getLimbY)(), std::string type,
	int sz_min, int sz_max) {
		using std::cout;

		cout << type << " : x = y : ";
		testIterate(opA, opB, getLimbX, getLimbY, sz_min, sz_max, doubleVar, doubleVar);
		cout << "\n";

		cout << type << " : x > y : ";
		testIterate(opA, opB, getLimbX, getLimbY, sz_min, sz_max, doubleVar, identity);
		cout << "\n";

		cout << type << " : x < y : ";
		testIterate(opA, opB, getLimbX, getLimbY, sz_min, sz_max, identity, doubleVar);
		cout << "\n";
}

void testIterate(BigInt (*opA)(BigInt&, BigInt&), BigInt (*opB)(BigInt&, BigInt&),
	uint32_t (*getLimbX)(), uint32_t (*getLimbY)(), size_t sz_min, size_t sz_max,
	void (*incX)(size_t&), void (*incY)(size_t&)) {
		using std::cout;

		size_t sz_x, sz_y;
		BigInt x, y;
		for (sz_x = sz_min, sz_y = sz_x; sz_y <= sz_max && sz_x <= sz_max; incX(sz_x), incY(sz_y)) {
			x.limbs.resize(sz_x);
			x.fill(getLimbX(), 0, x.numLimbs());

			y.limbs.resize(sz_y);
			y.fill(getLimbY(), 0, y.numLimbs());

			//cout << std::endl << "sz_x : " << sz_x << " sz_y : " << sz_y << std::endl;
			BigInt aC = opA(x, y);
			BigInt bC = opB(x, y);
			bool ok = aC == bC;
			if (!ok) {
				x.out("x.txt");
				y.out("y.txt");
				int bigLimbs = y.numLimbs() > x.numLimbs() ? y.numLimbs() : x.numLimbs();
				cout << bigLimbs << " : fail";
				throw "aC != bC in testIterate";
			}
		}
		int bigLimbs = y.numLimbs() > x.numLimbs() ? y.numLimbs() : x.numLimbs();
		std::cout << bigLimbs << " : ok";
}

uint32_t getF() {
	return 0xFFFFFFFF;
}

uint32_t getRand() {
	return rand() + (rand() << 16);
}

void doubleVar(size_t &var) {
	var = var * 2;
}

void identity(size_t &var) {
}

BigInt oclOp(void (*op)(oclBigInt&, oclBigInt&), BigInt &x, BigInt &y) {
	oclBigInt oX = x;
	oclBigInt oY = y;

	oclBigInt b;
	oX.copy(b);
	op(b, oY);
	b.truncate();
	return b.toBigInt();
}