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
	int s = degree / 8;
	if (s != 0) { // if we're shifting by bytes and not just bits
		for (int b = 0; b < data.size(); b++) {
			int i = b - s;
			if (i >= 0) {
				r.data[i] = r.data[b];
			}
			r.data[b] = 0;
		}
	}
	s = degree % 8;
	for (int b = 0; b < data.size(); b++) {
		if (b > 0) {
			unsigned char p = r.data[b] >> (8 - s);
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
	int s = degree / 8;
	if (s != 0) { // if we're shifting by bytes and not just bits
		for (int b = 0; b < data.size(); b++) {
			int i = b - s;
			if (i >= 0) {
				data[i] = data[b];
			}
			data[b] = 0;
		}
	}
	s = degree % 8;
	for (int b = 0; b < data.size(); b++) {
		if (b > 0) {
			unsigned char p = data[b] >> (8 - s);
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
	int s = degree / 8;
	if (s != 0) { // if we're shifting by bytes and not just bits
		for (int b = data.size() - 1; b >= 0; b--) {
			int i = b + s;
			while (i >= r.data.size()) {
				r.data.push_back(0);
			}
			r.data[i] = data[b];
			r.data[b] = 0;
		}
	}
	s = degree % 8;
	if (s != 0) {
		r.data.push_back(0);
		for (int b = r.data.size() - 2; b >= 0; b--) {
			unsigned char p = r.data[b] << (8 - s);
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
	int s = degree / 8;
	if (s != 0) { // if we're shifting by bytes and not just bits
		for (int b = data.size() - 1; b >= 0; b--) {
			int i = b + s;
			while (i >= data.size()) {
				data.push_back(0);
			}
			data[i] = data[b];
			data[b] = 0;
		}
	}
	s = degree % 8;
	if (s != 0) {
		data.push_back(0);
		for (int b = data.size() - 2; b >= 0; b--) {
			unsigned char p = data[b] << (8 - s);
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
		unsigned char p = r.data[b];
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
		unsigned char p = data[b];
		data[b] += _other.data[b];
		if (data[b] < p) {
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
			for (int i = 0; i < 8; i++) {
				if ((data[b] & (1 << i)) != 0) {
					t = _other;
					t <<= (i - b * 8);
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
	int numOk;
	for (numOk = 0; mask.data[numOk] == 0; numOk++) {
	}
	while (data.size() > numOk) {
		data.pop_back();
	}
}

std::string BigInt::get() const {
	std::string s;
	for (int i = 0; i < data.size(); i++) {
		if (i % 5 == 1) {
			s += " ";
		}
		unsigned char t = data.at(i);
		unsigned char n = t >> 4;
		if (i != 0 || n != 0) {
			s += intToHex(n);
		}
		n = t & 0xF;
		s += intToHex(n);
		if (i == 0) {
			s += ".";
		}
	}
	if (data.size() == 1) {
		s += "0";
	}
	return s;
}