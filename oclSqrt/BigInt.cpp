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

void BigInt::addCarry(BigInt carry) {
	cl_uint carry_g[512];
	const int workgroupSize = 2;

	for (int idx = 0; idx < workgroupSize; idx++) {
		carry_g[idx] = 0;
	}

	int tSize = carry.numLimbs();
	while (tSize > workgroupSize) {
		const int width = tSize / workgroupSize + (tSize % workgroupSize == 0 ? 0 : 1);
		for (int idx = 0; idx < workgroupSize; idx++) {
			carry_g[idx] = 0;
			cl_ulong curCarry = 0;
			const int start = idx * width;
			if (start < tSize) {
				int end = idx * width + width;
				end = end > tSize ? tSize : end;
				//curCarry = idx < (workgroupSize - 1) ? carry_g[idx + 1] : 0;
				curCarry = 0;
				for (int i = end - 1; i >= start; i--) {
					curCarry += carry.limbs[i];
					curCarry += limbs[i];
					limbs[i] = curCarry & 0xFFFFFFFF;
					carry.limbs[i] = 0;
					curCarry >>= 32;
				}
				carry_g[idx] = curCarry;
			}
		}
		// barrier

		for (int i = 1; i < workgroupSize; i++) {
			carry.limbs[i * width - 1] = carry_g[i];
		}

		int newSize;
		bool ok = true;
		for (newSize = workgroupSize; newSize > 0 && ok == true; newSize--) {
			if (carry_g[newSize - 1] != 0) {
				ok = false;
			}
		}
		tSize = newSize * width;
	}

	if (tSize > 0) {
		cl_ulong curCarry = 0;
		//cl_ulong curCarry = carry_g[1];
		for (int i = tSize - 1; i >= 0; i--) {
			curCarry += carry.limbs[i];
			curCarry += limbs[i];
			limbs[i] = curCarry & 0xFFFFFFFF;
			curCarry >>= 32;
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
		int start = curDigit + 1 - numLimbs();
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
		int start = curDigit + 1 - numLimbs();
		start = start < 0 ? 0 : start;
		//start = (start - curDigit > 
		int end = (curDigit < n.numLimbs() - 1 ? curDigit + 1 : n.numLimbs());
		int multiplicand = curDigit - start;
		unsigned long long curR = 0;
		unsigned int farCarry = 0; // if curR overflows
		//cout << endl << "start : " << start << " end : " << end << endl;
		for (int i = start; i < end; i++, multiplicand = curDigit - i) {
			//cout << "i : " << i << " m : " << multiplicand << " ";
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
		//cout << endl;
		r.limbs[curDigit] = (curR & 0xFFFFFFFF);
		if (curDigit > 0) {
			carry.limbs[curDigit - 1] += (curR >> 32);
			if (curDigit > 1) {
				carry2.limbs[curDigit - 2] += farCarry;
			}
		}
	}
	r += carry;
	r += carry2;
	return r;
}

BigInt BigInt::baseMul2(const BigInt &n) const {
	using std::cout; using std::endl;

	BigInt r(0U, n.numLimbs() + numLimbs() - 2);
	BigInt carry(0U, n.numLimbs() + numLimbs() - 2);
	BigInt carry2(0U, n.numLimbs() + numLimbs() - 2);
	for (int curDigit = 0; curDigit < r.numLimbs(); curDigit++) {
		// ********************
		// * interesting guts *
		// ********************
		int start = curDigit - (n.numLimbs() - 1);
		start = start < 0 ? 0 : start;
		int end = (curDigit < numLimbs() ? curDigit : numLimbs() - 1);
		uint64_t r_i = 0;
		uint32_t farCarry = 0; // if curR overflows
		//cout << endl << "start : " << start << " end : " << end << endl;
		for (int j = start; j <= end; j++) {
			//cout << "i : " << curDigit << " j : " << j << " m : " << (curDigit - j) << " ";
			uint64_t t = (uint64_t)limbs[j] * n.limbs[curDigit - j];
			if (r_i + t < r_i) {
				farCarry++;
			}
			r_i += t;
		}
		//cout << endl;
		r.limbs[curDigit] = (r_i & 0xFFFFFFFF);
		if (curDigit > 0) {
			carry.limbs[curDigit - 1] += (r_i >> 32);
			if (curDigit > 1) {
				carry2.limbs[curDigit - 2] += farCarry;
			}
		}
	}
	r += carry;
	r += carry2;
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

void BigInt::fill(unsigned int pattern, int ix_min, int ix_max) {
	for (int i = ix_min; i < ix_max; i++) {
		set(pattern, i);
	}
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
	for (int ixSrc = 0; ixSrc < bytes && ixSrc < limbs.size(); ixSrc++) {
		limbs[ixSrc] = 0;
	}
	for (int ixSrc = bytes; ixSrc < limbs.size(); ixSrc++) {
		int indexedLimb = ixSrc - bytes;
		int shifted = limbs[ixSrc] << bits;
		int overflow = limbs[ixSrc] >> (32 - bits);
		limbs[ixSrc] = 0;
		limbs[indexedLimb] = shifted;
		if (indexedLimb > 0 && bits > 0) {
			limbs[indexedLimb - 1] |= overflow;
		}
		
	}
	//int szNew = limbs.size() - bytes;
	//szNew = szNew < 0 ? 0 : szNew;
	//this->limbs.resize(szNew);

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
	//BigInt carry(0, n.limbs.size());
	for (size_t b = 0; b < n.limbs.size(); b++) {
		limb p = limbs[b];
		limb o = n.limbs[b];
		o += p;
		limbs[b] = o;
		if (o < p && b > 0) {
			//carry.limbs[b - 1] ++;
			carry(b - 1);
		}
	}
	return *this;
}

BigInt BigInt::operator-(const BigInt &n) const {
	//BigInt t = n.neg();
	//std::cout << t << std::endl;
	return *this + n.neg();
}
BigInt &BigInt::operator-=(const BigInt &n) {
	return *this += n.neg();
}

BigInt BigInt::operator*(const BigInt &n) const {
	using std::cout; using std::endl;
	const int minSize = 0x200; // must be > 4

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (numLimbs() < minSize && n.numLimbs() < minSize) {
		BigInt r = this->baseMul(n);
		r.truncate();
		return r;
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

// receives this & n *x^(1-p), returns r *x^(-2-2p)
BigInt BigInt::shiftMul(const BigInt &n, int minSize) const {
	using std::cout; using std::endl;
	//const int minSize = 0x2; // must be > 4

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (numLimbs() < minSize && n.numLimbs() < minSize) {
		return (*this >> 64).baseMul(n >> 64);
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
	a_1.limbs.resize(p); // m_1 * x^(1 - p)

	BigInt a_0 = *this;
	for (int i = 0; i < p; i++) {
		a_0.set(0, i);
	}
	a_0 <<= (p * 32);
	a_0.limbs.resize(p);

	BigInt b_1 = n;
	for (int i = p; i < n.numLimbs(); i++) {
		b_1.set(0, i);
	}
	b_1.limbs.resize(p); // n_1 * x^(1 - p)

	BigInt b_0 = n;
	for (int i = 0; i < p; i++) {
		b_0.set(0, i);
	}
	b_0 <<= (p * 32);
	b_0.limbs.resize(p); // n_0 * x^(1 - p)

	BigInt c_2 = a_1.shiftMul(b_1, minSize); // c_2 = m_1 * n_1 * x^(-2 - 2p)
	BigInt c_0 = a_0.shiftMul(b_0, minSize); // c_0 = m_0 * n_0 * x^(-2 - 2p)

	//std::ofstream file("cpu.txt");

	//file << "a_1 : " << a_1 << endl;
	//file << "a_0 : " << a_0 << endl;
	//file << "b_1 : " << b_1 << endl;
	//file << "b_0 : " << b_0 << endl;
	//file << "c_2 : " << c_2 << endl;
	//file << "c_0 : " << c_0 << endl;

	a_1 >>= 32;                             // m_1 * x^(-p)
	a_0 >>= 32;                             // m_0 * x^(-p)
	BigInt c_1 = (a_1 + a_0); 
	BigInt o_a = c_1.limbs[0]; // overflow of addition

	b_1 >>= 32;                             // n_1 * x^(-p)
	b_0 >>= 32;                             // n_0 * x^(-p)
	BigInt t = (b_1 + b_0); 
	BigInt o_b = t.limbs[0]; // overflow of addition

	BigInt t_a, t_b, t_c;
	t_a =  t;
	t_a.limbs[0] = 0;
	t_b = c_1;
	t_b.limbs[0] = 0;
	t_c = o_a;

	t_a = t_a.baseMul(o_a);
	t_b = t_b.baseMul(o_b);
	t_c = t_c.baseMul(o_b);
	t_a >>= 64;
	t_b >>= 64;
	t_c >>= 64;

	c_1 <<= 32; // c_1 = (m_1 + m_0) * x^(1-p)
	c_1.limbs.resize(p);
	t <<= 32;  // t = (n_1 + n_0) * x^(1-p)
	t.limbs.resize(p);

	//cout << "o_a : " << o_a << endl
	//	<< "o_b : " << o_b << endl
	//	<< "c_1 : " << c_1 << endl
	//	<< "t_a : " << t_a << endl
	//	<< "t_b : " << t_b << endl
	//	<< "t_c : " << t_c << endl;

	c_1 = c_1.shiftMul(t, minSize);               // (m_1 + m_0)(n_1 + n_0) * x^(-2 - 2p)
	//cout << "shiftMul : " << c_1 << endl;
	c_1 += t_a;
	//cout << "+ t_a : " << c_1 << endl;
	c_1 += t_b;
	//cout << "+ t_b : " << c_1 << endl;
	c_1 += t_c;
	//cout << "+ t_c : " << c_1 << endl;
	c_1 -= c_2;                             // ((m_1 + m_0)(n_1 + n_0) - r_2) * x^(-2 - 2p)
	//cout << "- c_2 : " << c_1 << endl;
	c_1 -= c_0;                             // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(-2 - 2p)
	//cout << "- c_0 : " << c_1 << endl;
	//cout << endl;
	//file.close();

	//c_2 <<= 4 * 32;     // c_2 = m_1 * n_1 * x^(-2-2p)
	c_0 >>= (2 * p) * 32; // c_0 = m_0 * n_0 * x^(-2 - 4p)
	c_1 >>= p * 32;       // c_1 = ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(-2 - 3p)

	c_0 += c_2; // c_0 = R
	c_0 += c_1;
	return c_0;
}

BigInt BigInt::mulDigit(const BigInt &n, int minSize) const {
	using std::cout; using std::endl;
	//const int minSize = 0x8; // must be >= 2

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

	BigInt r = this->shiftMul(n, minSize);
	r = r <<= (4 * 32);
	r.limbs.resize(-1 + 4 * p);
	return r;
}

bool BigInt::operator==(const BigInt &n) const {
	if(numLimbs() != n.numLimbs()) {
		return false;
	}
	for (int i = 0; i < numLimbs(); i++) {
		if (limbs[i] != n.limbs[i]) {
			std::vector<cl_uint> t(3);
			std::cout << "conflict->";
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

void BigInt::out(const char *fileName) {
	std::ofstream file(fileName, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (file.is_open() == true) {
		file << limbs.size() << " ";
		for (int i = 0; i < limbs.size(); i++) {
			file << limbs[i] << " ";
		}
		file.close();
	} else {
		std::cout << "file could not be opened." << std::endl;
		std::cin.get();
	}
}

void BigInt::in(const char *fileName) {
	std::ifstream file(fileName, std::ios_base::in | std::ios_base::binary);
	if (file.is_open() == true) {
		int newSize;
		file >> newSize;
		limbs.resize(newSize);
		for (int i = 0; i < limbs.size(); i++) {
			file >> limbs[i];
		}
		file.close();
	} else {
		std::cout << "file could not be opened." << std::endl;
		std::cin.get();
	}
}

std::ostream& operator<<(std::ostream &os, const BigInt &n) {
	os << n.get().c_str();
	return os;
}