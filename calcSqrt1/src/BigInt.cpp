#include "StdAfx.h"
#include "BigInt.h"

void BigInt::carry(const int index) {
	if (index >= 0) {
		data[index] += 1;
		if(index > 0 && data[index] == 0) {
			carry(index - 1);
		}
	}
}

char BigInt::intToHex(unsigned char input) const {
	if (input > 9) {
		input += 55;
	} else {
		input += 48;
	}
	return input;
}

BigInt::~BigInt(void)
{
}

BigInt BigInt::operator<<(const int degree) const {
	BigInt r = *this;
	if (degree < 0) {
		return r >> (degree * -1);
	}
	int s = degree / BIGINT_SIZE;
	if (s != 0) { // if we're shifting by chunks and not just bits
		for (int b = 0; b < data.size(); b++) {
			int i = b - s;
			if (i >= 0) {
				r.data[i] = r.data[b];
			}
			r.data[b] = 0;
		}
	}
	s = degree % BIGINT_SIZE;
	for (int b = 0; b < data.size(); b++) {
		if (b > 0) {
			chunk p = r.data[b] >> (BIGINT_SIZE - s);
			r.data[b - 1] |= p;
		}
		r.data[b] <<= s;
	}
	return r;
}

BigInt &BigInt::operator<<=(const int degree) {
	if (degree < 0) {
		*this >>= (degree * -1);
		return *this;
	}
	int s = degree / BIGINT_SIZE;
	if (s != 0) { // if we're shifting by chunks and not just bits
		for (int b = 0; b < data.size(); b++) {
			int i = b - s;
			if (i >= 0) {
				data[i] = data[b];
			}
			data[b] = 0;
		}
	}
	s = degree % BIGINT_SIZE;
	for (int b = 0; b < data.size(); b++) {
		if (b > 0) {
			chunk p = data[b] >> (BIGINT_SIZE - s);
			data[b - 1] |= p;
		}
		data[b] <<= s;
	}
	return *this;
}

BigInt BigInt::operator>>(const int degree) const {
	BigInt r = *this;
	if (degree < 0) {
		return r << (degree * -1);
	}
	int s = degree / BIGINT_SIZE;
	if (s != 0) { // if we're shifting by chunks and not just bits
		for (int b = data.size() - 1; b >= 0; b--) {
			int i = b + s;
			while (i >= r.data.size()) {
				r.data.push_back(0);
			}
			r.data[i] = data[b];
			r.data[b] = 0;
		}
	}
	s = degree % BIGINT_SIZE;
	if (s != 0) {
		r.data.push_back(0);
		for (int b = r.data.size() - 2; b >= 0; b--) {
			chunk p = r.data[b] << (BIGINT_SIZE - s);
			r.data[b + 1] |= p;
			r.data[b] >>= s;
		}
	}
	return r;
}

BigInt &BigInt::operator>>=(const int degree) {
	if (degree < 0) {
		*this <<= (degree * -1);
		return *this;
	}
	int s = degree / BIGINT_SIZE;
	if (s != 0) { // if we're shifting by chunks and not just bits
		for (int b = data.size() - 1; b >= 0; b--) {
			int i = b + s;
			while (i >= data.size()) {
				data.push_back(0);
			}
			data[i] = data[b];
			data[b] = 0;
		}
	}
	s = degree % BIGINT_SIZE;
	if (s != 0) {
		data.push_back(0);
		for (int b = data.size() - 2; b >= 0; b--) {
			chunk p = data[b] << (BIGINT_SIZE - s);
			data[b + 1] |= p;
			data[b] >>= s;
		}
	}
	return *this;
}

BigInt BigInt::operator+(const BigInt &_other) const {
	BigInt r = *this;
	while (_other.data.size() > r.data.size()) {
		r.data.push_back(0);
	}
	for (int b = 0; b < _other.data.size(); b++) {
		chunk p = r.data[b];
		r.data[b] += _other.data[b];
		if (r.data[b] < p) {
			r.carry(b - 1);
		}
	}
	return r;
}

BigInt BigInt::operator+(const int other) const {
	BigInt r = *this;
	if (data.size() < 1) {
		r.data.push_back(other);
	} else {
		r.data[0] += other;
	}
	return r;
}

BigInt &BigInt::operator+=(const BigInt &_other) {
	while (_other.data.size() > data.size()) {
		data.push_back(0);
	}
	for (int b = 0; b < _other.data.size(); b++) {
		chunk p = data[b];
		chunk o = _other.data[b];
		o += p;
		data[b] = o;
		if (o < p) {
			carry(b - 1);
		}
	}
	return *this;
}

BigInt BigInt::operator*(const BigInt &_other) const {
	BigInt r;
	BigInt t;
	for (int b = 0; b < data.size(); b++) {
		if (data[b]) {
			for (int i = 0; i < BIGINT_SIZE; i++) {
				if ((data[b] & (1 << i)) != 0) {
					t = _other;
					t <<= (i - b * BIGINT_SIZE);
					r += t;
				}
			}
		}
	}
	return r;
}

BigInt BigInt::operator~() const {
	BigInt r = *this;
	for (int i = 0; i < data.size(); i++) {
		r.data[i] = ~(data[i]);
	}
	return r;
}

void BigInt::rmask(const BigInt &mask) {
	int chunksOk;
	for (chunksOk = 0; mask.data[chunksOk] == 0; chunksOk++) {
	}
	int digitsOk;
	chunk t = mask.data[chunksOk] & (0xF << (BIGINT_SIZE - 4));
	for (digitsOk = 0; (mask.data[chunksOk] & (0xFU << (BIGINT_SIZE - 4 - digitsOk * 4))) == 0; digitsOk++) {
	}
	data[chunksOk] &= (~0 << (BIGINT_SIZE - 4 * digitsOk));
	chunksOk++;
	while (data.size() > chunksOk) {
		data.pop_back();
	}
}

std::string BigInt::get() const {
	std::string s;
	int chars = 0;
	for (int i = 0; i < data.size(); i++) {
		chunk t = data[i];
		for (int j = (BIGINT_SIZE / 4) - 1; j >= 0; j--) {
			unsigned char n = t >> (j * 4);
			n &= 0xF;
			if (i != 0 || n != 0 || j == 0) {
				s += intToHex(n);
				if (i != 0) {
					chars++;
				}
				if (chars % 10 == 0) {
					s += " ";
				}
			}
		}
		if (i == 0) {
			s += ". ";
		}
	}
	if (data.size() == 1) {
		s += "0";
	}
	return s;
}

//std::string BigInt::get() const {
//	std::string s;
//	for (int i = 0; i < data.size(); i++) {
//		if (i % 5 == 1) {
//			s += " ";
//		}
//		chunk t = data.at(i);
//		chunk n = t >> 4;
//		if (i != 0 || n != 0) {
//			s += intToHex(n);
//		}
//		n = t & 0xF;
//		s += intToHex(n);
//		if (i == 0) {
//			s += ".";
//		}
//	}
//	if (data.size() == 1) {
//		s += "0";
//	}
//	return s;
//}