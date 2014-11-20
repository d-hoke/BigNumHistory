#include "StdAfx.h"
#include "BigInt.h"

void BigInt::truncate() {
	int i;
	for (i = limbs.size(); i > 1 && limbs[i - 1] == 0; i--) {
	}
	limbs.resize(i);
}

void BigInt::carry(const int index) {
	if (index >= 0) {
		limbs[index] += 1;
		if (index > 0 && limbs[index] == 0) {
			carry(index - 1);
		}
	}
}

void BigInt::rmask(const BigInt &mask) {
	int limbsOk;
	for (limbsOk = 0; mask.limbs[limbsOk] == 0; limbsOk++) {
	}
	int digitsOk;
	limb t = mask.limbs[limbsOk] & (0xF << (BIGINT_LIMB_SIZE - 4));

	for (digitsOk = 0; (mask.limbs[limbsOk] & (0xF << (BIGINT_LIMB_SIZE - 4 - digitsOk * 4))) == 0; digitsOk++) {
	}
	if (digitsOk > 0) {
		limbs[limbsOk] &= (~0 << (BIGINT_LIMB_SIZE - 4 * digitsOk));
		limbsOk++;
	}
	while (limbs.size() > limbsOk) {
		limbs.pop_back();
	}
}

BigInt BigInt::oldMul(const BigInt &n) const {
	unsigned int largeSize = (n.numLimbs() > numLimbs() ? n.numLimbs() : numLimbs()) - 1;
	BigInt r(0U, n.numLimbs() + numLimbs() - 2);
	BigInt carry(0U, n.numLimbs() + numLimbs() - 2);
	for (int curDigit = 0; curDigit < r.numLimbs(); curDigit++) {
		int start = curDigit - largeSize;
		start = start < 0 ? 0 : start;
		int end = (curDigit < n.numLimbs() - 1 ? curDigit + 1 : n.numLimbs());
		//int end = n.numLimbs();
		int multiplicand = curDigit - start;
		for (int i = start; i < end && multiplicand < numLimbs(); i++, multiplicand = curDigit - i) {
			for (int curBit = 0; curBit < 32; curBit++) {
				if ((n.limbs[i] & (1 << curBit)) != 0) {
					unsigned int addend = 0;
					addend |= (limbs[multiplicand] << curBit);
					// in ocl, break out of loop
					if (multiplicand + 1 < limbs.size() && curBit != 0) {
						addend |= (limbs[multiplicand + 1] >> (32 - curBit));
					}
					if (addend > 0) {
						unsigned int p = r.limbs[curDigit];
						r.limbs[curDigit] += addend;
						if (r.limbs[curDigit] < p && curDigit > 0) {
							carry.limbs[curDigit - 1]++;
							if (carry.limbs[curDigit - 1] == 0) {
								std::cout << "stop" << std::endl;
							}
						}
					}
				}
			}
		}
		if (curDigit < n.numLimbs() - 1) {
			int multiplicand = -1;
			for (int curBit = 0; curBit < 32; curBit++) {
				if ((n.limbs[curDigit + 1] & (1 << curBit)) != 0) {
					// in ocl, break final out of loop
					unsigned int addend = 0;
					if (multiplicand + 1 < limbs.size() && curBit != 0) {
						addend |= (limbs[multiplicand + 1] >> (32 - curBit));
					}
					if (addend > 0) {
						unsigned int p = r.limbs[curDigit];
						r.limbs[curDigit] += addend;
						if (r.limbs[curDigit] < p && curDigit > 0) {
							carry.limbs[curDigit - 1]++;
							if (carry.limbs[curDigit - 1] == 0) {
								std::cout << "stop" << std::endl;
							}
						}
					}
				}
			}
		}
	}
	r += carry;
	return r;
}

BigInt BigInt::baseMul(const BigInt &n) const {
	using std::cout; using std::endl;

	unsigned int largeSize = (n.numLimbs() > numLimbs() ? n.numLimbs() : numLimbs()) - 1;
	BigInt r(0U, n.numLimbs() + numLimbs() - 2);
	BigInt carry(0U, n.numLimbs() + numLimbs() - 2);
	BigInt carry2(0U, n.numLimbs() + numLimbs() - 2);
	for (int curDigit = 0; curDigit < r.numLimbs(); curDigit++) {
		// ********************
		// * interesting guts *
		// ********************
		int start = curDigit - largeSize;
		start = start < 0 ? 0 : start;
		int end = (curDigit < n.numLimbs() - 1 ? curDigit + 1 : n.numLimbs());
		int multiplicand = curDigit - start;
		unsigned long long curR = 0;
		unsigned int farCarry = 0; // if curR overflows
		for (int i = start; i < end && multiplicand < numLimbs(); i++, multiplicand = curDigit - i) {
			unsigned long long p = curR;
			unsigned long long t = (unsigned long long)n.limbs[i] * limbs[multiplicand];
			curR += t;
			if (curR < p) {
				farCarry ++;
				if (farCarry == 0) {
					std::cout << "stop." << std::endl;
				}
			}

		}
		r.limbs[curDigit] = (curR & 0xFFFFFFFF);
		if (curDigit > 0) {
			carry.limbs[curDigit - 1] += (curR >> 32);
			if (curDigit > 1) {
				carry2.limbs[curDigit - 2] += farCarry;
			}
		}
	}
	carry += carry2;
	r += carry;
	return r;
}

BigInt::BigInt(void)
{
}

BigInt::BigInt(unsigned int i, unsigned int pos) {
	limbs.resize(pos + 1, 0U);
	limbs[pos] = (limb)i;
}

BigInt::BigInt(double d, int prec) {
	for (int i = 0; i < prec; i++) {
		limb t = (limb)(floor(d));
		limbs.push_back(t);
		d -= t;
		d *= pow(2.0, BIGINT_LIMB_SIZE);
	}
}

BigInt::BigInt(const cl_mem buffer, const size_t numLimbs, const cl_command_queue queue) {
	limbs.resize(numLimbs);
	cl_int error = clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, numLimbs * sizeof(unsigned int), &(limbs[0]), 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error converting oclBigInt to BigInt: " << error << std::endl;
		std::cin.get();
		exit(error);
	}
}

BigInt::~BigInt(void)
{
}

void BigInt::set(unsigned int i, unsigned int pos) {
	if (pos >= limbs.size()) {
		limbs.resize(pos + 1, 0U);
	}
	limbs[pos] = (limb)i;
}

BigInt BigInt::operator<<(const int d) const {
	BigInt r = *this;
	if (d < 0) {
		return r >> (d * -1);
	}
	return r <<= d;
}

BigInt &BigInt::operator<<=(const int d) {
	if (d < 0) {
		*this >>= (d * -1);
		return *this;
	}
	else if (d == 0) {
		return *this;
	}

	const int bytes = d / 32;
	const int bits = d % 32;
	for (int i = 0; i < limbs.size(); i++) {
		int indexedLimb = i - bytes;
		if (indexedLimb >= 0) {
			int shifted = limbs[i] << bits;
			int overflow = limbs[i] >> (32 - bits);
			limbs[indexedLimb] = shifted;
			if (indexedLimb > 0 && bits > 0) {
				limbs[indexedLimb - 1] |= overflow;
			}
		}
		limbs[i] = 0;
	}

	return *this;
}

BigInt BigInt::operator>>(const int d) const {
	BigInt r = *this;
	if (d < 0) {
		return r << (d * -1);
	}
	return r >>= d;
}

BigInt &BigInt::operator>>=(const int d) {
	if (d < 0) {
		*this <<= (d * -1);
		return *this;
	} else if (d == 0) {
		return *this;
	}
	int s = d / BIGINT_LIMB_SIZE;
	if (s != 0) { // if we're shifting by limbs and not just bits
		int b = limbs.size() - 1;
		size_t newSize = b + 1 + s;
		while (newSize > limbs.size()) {
			limbs.push_back(0);
		}
		for (; b >= 0; b--) {
			limbs[b + s] = limbs[b];
			limbs[b] = 0;
		}
	}
	s = d % BIGINT_LIMB_SIZE;
	if (s != 0) {
		limbs.push_back(0);
		for (int b = limbs.size() - 2; b >= 0; b--) {
			limb p = limbs[b] << (BIGINT_LIMB_SIZE - s);
			limbs[b + 1] |= p;
			limbs[b] >>= s;
		}
	}
	return *this;
}

BigInt BigInt::operator+(const BigInt &n) const {
	BigInt r = *this;
	return r += n;
}

BigInt &BigInt::operator+=(const BigInt &n) {
	if (n.limbs.size() > limbs.size()) {
		limbs.resize(n.limbs.size(), 0);
	}
	for (size_t b = 0; b < n.limbs.size(); b++) {
		limb p = limbs[b];
		limb o = n.limbs[b];
		o += p;
		limbs[b] = o;
		if (o < p) {
			carry(b - 1);
		}
	}
	return *this;
}

BigInt BigInt::operator-(const BigInt &n) const {
	return *this + n.neg();
}
BigInt &BigInt::operator-=(const BigInt &n) {
	return *this += n.neg();
}

BigInt BigInt::operator*(const BigInt &n) const {
	using std::cout; using std::endl;
	const int minSize = 0x8; // must be > 4

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (numLimbs() < minSize && n.numLimbs() < minSize) {
		return this->baseMul(n);
	}

	int p;
	// temp way to match size
	if (n.numLimbs() > numLimbs()) { // num limbs / 2 rounded up
		p = n.numLimbs() / 2 + n.numLimbs() % 2;
	} else {
		p = numLimbs() / 2 + numLimbs() % 2;
	}

	BigInt a_1 = *this;
	for (int i = p; i < numLimbs(); i++) {
		a_1.set(0, i);
	}
	a_1 >>= (32 + 32);
	a_1.truncate(); // m_1 / x^(p + 1)

	BigInt a_0 = *this;
	for (int i = 0; i < p; i++) {
		a_0.set(0, i);
	}
	a_0 <<= (p * 32 - 32 - 32);
	a_0.truncate(); // m_0 / x^(p + 1)

	BigInt b_1 = n;
	for (int i = p; i < n.numLimbs(); i++) {
		b_1.set(0, i);
	}
	b_1 >>= (32 + 32);
	b_1.truncate(); // n_1 / x^(p + 1)

	BigInt b_0 = n;
	for (int i = 0; i < p; i++) {
		b_0.set(0, i);
	}
	b_0 <<= (p * 32 - 32 - 32);
	b_0.truncate(); // n_0 / x^(p + 1)

	BigInt c_2 = a_1 * b_1; // c_2 = m_1 * n_1 / x^(2p + 2)
	BigInt c_0 = a_0 * b_0; // c_0 = m_0 * n_0 / x^(2p + 2)

	BigInt c_1 = (a_1 + a_0);
	BigInt t = (b_1 + b_0); 
	c_1 = c_1 * t;                          // (m_1 + m_0)(n_1 + n_0) * x^(-2p - 2)
	c_1 -= c_2;                             // ((m_1 + m_0)(n_1 + n_0) - r_2) * x^(-2p - 2)
	c_1 -= c_0;                             // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(-2p - 2)

	c_2 <<= 4 * 32;        // c_2 = m_1 * n_1 * x^(2-2p)
	c_0 >>= (2 * p - 4) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
	c_1 >>= (p - 4) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

	c_0 += c_2; // c_0 = R
	c_0 += c_1;
	c_0.truncate();
	return c_0;
}

BigInt BigInt::mulDigit(const BigInt &n) const {
	using std::cout; using std::endl;
	const int minSize = 0x8; // must be > 4

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (numLimbs() < minSize && n.numLimbs() < minSize) {
		return this->baseMul(n);
	}

	int p;
	// temp way to match size
	if (n.numLimbs() > numLimbs()) { // num limbs / 2 rounded up
		p = n.numLimbs() / 2 + n.numLimbs() % 2;
	} else {
		p = numLimbs() / 2 + numLimbs() % 2;
	}

	//if (p < minSize) {
		BigInt a_1 = *this;
		for (int i = p; i < numLimbs(); i++) {
			a_1.set(0, i);
		}
		//a_1 >>= (32 + 32);
		a_1.limbs.resize(p); // m_1 * x^(1 - p)
		//a_1.truncate(); 

		BigInt a_0 = *this;
		for (int i = 0; i < p; i++) {
			a_0.set(0, i);
		}
		a_0 <<= (p * 32 - 32 - 32);
		a_0.truncate(); // m_0 / x^(p + 1)

		BigInt b_1 = n;
		for (int i = p; i < n.numLimbs(); i++) {
			b_1.set(0, i);
		}
		//b_1 >>= (32 + 32);
		b_1.limbs.resize(p); // n_1 * x^(1 - p)
		//b_1.truncate(); 

		BigInt b_0 = n;
		for (int i = 0; i < p; i++) {
			b_0.set(0, i);
		}
		b_0 <<= (p * 32 - 32 - 32);
		b_0.truncate(); // n_0 / x^(p + 1)

		BigInt c_2 = a_1.mulDigit(b_1); // c_2 = m_1 * n_1 * x^( 2 - 2p)
		BigInt c_0 = a_0.mulDigit(b_0); // c_0 = m_0 * n_0 * x^(-2 - 2p)

		a_1 >>= 64;                             // m_1 * (-1 - p)
		BigInt c_1 = (a_1 + a_0);
		b_1 >>= 64;                             // n_1 * (-1 - p)
		BigInt t = (b_1 + b_0); 
		c_1 = c_1.mulDigit(t);                  // (m_1 + m_0)(n_1 + n_0) * x^(-2p - 2)
		c_2 >>= 128;
		c_1 -= c_2;                             // ((m_1 + m_0)(n_1 + n_0) - r_2) * x^(-2p - 2)
		c_1 -= c_0;                             // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(-2p - 2)

		c_2 <<= 4 * 32;        // c_2 = m_1 * n_1 * x^(2-2p)
		c_0 >>= (2 * p - 4) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
		c_1 >>= (p - 4) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

		c_0 += c_2; // c_0 = R
		c_0 += c_1;
		c_0.truncate();
		return c_0;
	//} else {
	//	BigInt a_1 = *this;
	//	for (int i = p; i < numLimbs(); i++) {
	//		a_1.set(0, i);
	//	}
	//	//a_1 >>= (32 + 32);
	//	a_1.truncate(); // m_1 * x^(1 - p)

	//	BigInt a_0 = *this;
	//	for (int i = 0; i < p; i++) {
	//		a_0.set(0, i);
	//	}
	//	a_0 <<= (p * 32);
	//	a_0.truncate(); // m_0 * x^(1 - p)

	//	BigInt b_1 = n;
	//	for (int i = p; i < n.numLimbs(); i++) {
	//		b_1.set(0, i);
	//	}
	//	//b_1 >>= (32 + 32);
	//	b_1.truncate(); // n_1 * x^(1 - p)

	//	BigInt b_0 = n;
	//	for (int i = 0; i < p; i++) {
	//		b_0.set(0, i);
	//	}
	//	b_0 <<= (p * 32);
	//	b_0.truncate(); // n_0 * x^(1 - p)

	//	BigInt c_2 = a_1.mulDigit(b_1); // c_2 = m_1 * n_1 / x^(2p + 2)
	//	BigInt c_0 = a_0.mulDigit(b_0); // c_0 = m_0 * n_0 / x^(2p + 2)

	//	BigInt c_1 = (a_1 + a_0);
	//	BigInt t = (b_1 + b_0); 
	//	c_1 = c_1.mulDigit(t);          // (m_1 + m_0)(n_1 + n_0) * x^(2 - 2p)
	//	c_1 -= c_2;                     // ((m_1 + m_0)(n_1 + n_0) - r_2) * x^(2 - 2p)
	//	c_1 -= c_0;                     // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(2 - 2p)

	//	//c_2 <<= 4 * 32;        // c_2 = m_1 * n_1 * x^(2-2p)
	//	c_0 >>= (2 * p) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
	//	c_1 >>= (p) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

	//	c_0 += c_2; // c_0 = R
	//	c_0 += c_1;
	//	c_0.truncate();
	//	return c_0;
	//}
}

bool BigInt::operator==(const BigInt &n) const {
	if(numLimbs() != n.numLimbs()) {
		return false;
	}
	for (int i = 0; i < numLimbs(); i++) {
		if (limbs[i] != n.limbs[i]) {
			std::cout << "stupid." << std::endl;
			return false;
		}
	}
	return true;
}

bool BigInt::operator>(const unsigned int other) const {
	if (limbs[0] > other || (limbs[0] == other && numLimbs() > 1)) {
		return true;
	}
	return false;
}

BigInt BigInt::neg() const {
	BigInt r = *this;
	return r.setNeg();
}

void BigInt::verify() {
	BigInt e = BigInt(1U) - ((*this * *this) << 1);
	rmask(e);
}

BigInt &BigInt::setNeg() {
	for (int i = 0; i < limbs.size(); i++) {
		limbs[i] = ~(limbs[i]);
	}
	carry(limbs.size() - 1);
	return *this;
}

std::string BigInt::get() const {
	std::string s;
	int chars = 0;
	BigInt n = *this;
	if (n.limbs.size() > 0 && (n.limbs[0] & (1U << (BIGINT_LIMB_SIZE - 1))) != 0) {
		//n.setNeg();
		//s += "-";
	}
	for (size_t i = 0; i < n.limbs.size(); i++) {
		limb iLimb = n.limbs[i];
		for (int j = (BIGINT_LIMB_SIZE / 4) - 1; j >= 0; j--) {
			unsigned char digit = iLimb >> (j * 4);
			digit &= 0xF;
			if (i != 0 || digit != 0 || j == 0 || iLimb != 0) {
				if (i != 0) {
					if (chars % 8 == 0) {
						s += " ";
					}
					chars++;
				}
				s += intToHex(digit);
			}
		}
		if (i == 0) {
			s += ".";
		}
	}
	if (n.limbs.size() == 1) {
		s += " 0";
	}

	return s;
}

std::ostream& operator<<(std::ostream &os, const BigInt &n) {
	os << n.get().c_str();
	return os;
}