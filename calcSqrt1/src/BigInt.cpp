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

void BigInt::truncate() {
	for (int i = data.size() - 1; i > 0 && data[i] == 0; i--) {
		data.pop_back();
	}
}

BigInt BigInt::getTruncate() const {
	BigInt r = *this;
	r.truncate();
	return r;
}

void BigInt::rmask(const BigInt &mask) {
	int chunksOk;
	for (chunksOk = 0; mask.data[chunksOk] == 0; chunksOk++) {
	}
	int bitsOk;
	bool isOk = true;
	for (bitsOk = 0; isOk == true; bitsOk++) {
		if (bitsOk == 32) {
			std::cout << "stop";
		}
		unsigned int x = 1U << (32 - bitsOk - 1);
		if ((mask.data[chunksOk] & x) != 0) {
			isOk = false;
		}
	}
	if (bitsOk > 0) {
		data[chunksOk] &= (~0 << (32 - bitsOk));
		chunksOk++;
	}
	//int digitsOk;
	//chunk t = mask.data[chunksOk] & (0xF << (BIGINT_SIZE - 4));
	//for (digitsOk = 0; (mask.data[chunksOk] & (0xF << (BIGINT_SIZE - 4 - digitsOk * 4))) == 0; digitsOk++) {
	//}
	//if (digitsOk > 0) {
	//	data[chunksOk] &= (~0 << (BIGINT_SIZE - 4 * digitsOk));
	//	chunksOk++;
	//}
	while (data.size() > chunksOk) {
		data.pop_back();
	}
}

BigInt::~BigInt(void)
{
}

BigInt BigInt::operator<<(const int degree) const {
	BigInt r = *this;
	if (degree < 0) {
		return r >> (degree * -1);
	}
	return r <<= degree;
}

BigInt &BigInt::operator<<=(const int degree) {
	if (degree < 0) {
		*this >>= (degree * -1);
		return *this;
	}
	else if (degree == 0) {
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
	if (s > 0) {
		for (int b = 0; b < data.size(); b++) {
			if (b > 0) {
				chunk p = data[b] >> (BIGINT_SIZE - s);
				data[b - 1] |= p;
			}
			data[b] <<= s;
		}
	}

	return *this;
}

BigInt BigInt::operator>>(const int degree) const {
	BigInt r = *this;
	if (degree < 0) {
		return r << (degree * -1);
	}
	return r >>= degree;
}

BigInt &BigInt::operator>>=(const int degree) {
	if (degree < 0) {
		*this <<= (degree * -1);
		return *this;
	} else if (degree == 0) {
		return *this;
	}
	int s = degree / BIGINT_SIZE;
	if (s != 0) { // if we're shifting by chunks and not just bits
		int b = data.size() - 1;
		int newSize = b + 1 + s;
		while (newSize > data.size()) {
			data.push_back(0);
		}
		for (; b >= 0; b--) {
			data[b + s] = data[b];
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
		if (b == 0xd) {
			int x = 0;
		}
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

BigInt BigInt::operator-(const BigInt &n) const {
	return *this + ~n;
}
BigInt &BigInt::operator-=(const BigInt &n) {
	return *this += ~n;
}

BigInt BigInt::operator*(const BigInt &_other) const {
	BigInt r;
	BigInt t;
	for (int b = 0; b < data.size(); b++) {
		if (data[b]) {
			for (int i = 0; i < BIGINT_SIZE; i++) {
				if ((data[b] & (1U << i)) != 0) {
					t = _other;
					t <<= (i - b * BIGINT_SIZE);
					r += t;

				}
			}
		}
	}
	return r;
}

BigInt BigInt::operator*(const int other) const {
	BigInt r;
	BigInt t;
	for (int i = 0; i < 31; i++) {
		if ((other & (1 << i)) != 0) {
			t = *this;
			t <<= i;
			r += t;
		}
	}
	return r;
}

BigInt BigInt::operator~() const {
	BigInt r = *this;
	for (int i = 0; i < r.data.size(); i++) {
		r.data[i] = ~(r.data[i]);
	}
	r.carry(r.data.size() - 1);
	return r;
}

BigInt BigInt::squared(const int minSize) const {
	using std::endl; using std::cout;
	//const int minSize = 128;

	BigInt a_1, a_0;
	this->getTruncate().split(a_1, a_0);

	int o = (a_0.data.size() - 1) * BIGINT_SIZE;
	int m = (a_1.data.size() - 1) * BIGINT_SIZE;
	//cout << "a_0 = " << a_0.get() << "\na_1 = " << a_1.get() << "\no = " << o << "\nm = " << m << endl;
	BigInt z_0, z_2;
	if (a_1.data.size() > minSize) {
		//cout << "split a_1!\n";
		//cout << endl;
		z_2 = a_1.squared(minSize);
		//cout << "a_1 = " << a_1.get() << "\na_1 * a_1 = " << (a_1 * a_1).get() << "\na_1^2 = " << z_2.get() << endl << endl;
	} else {
		z_2 = a_1 * a_1;
	}
	if ((a_0.data.size() - a_1.data.size()) > minSize) {
		//cout << "split a_0!\n";
		//cout << endl;
		//z_0 = (a_0 << (o-m)).squared() >> (2*(o-m));
		//cout << "a_0 = " << a_0.get() << endl;
		z_0 = (a_0 << (o-m - 2*BIGINT_SIZE));
		//cout << "z_0 = a_0 << (o-m-1) = " << z_0.get() << endl;
		z_0 = z_0.squared(minSize);
		//cout << "z_0^2 = " << z_0.get() << endl;
		z_0 = z_0 >> (2*(o-m - 2*BIGINT_SIZE));
		//cout << "z_0 >> (2*(o-m-1)) = " << z_0.get() << endl;
		//cout << "a_0 = " << a_0.get() << "\na_0 * a_0 = " << (a_0 * a_0).get() << "\na_0^2 = " << z_0.get() << endl << endl;
	} else {
		z_0 = a_0 * a_0;
	}
	//BigInt z_1 = (a_1 + (a_0 << (o - m))) << ((m - o) / 2);
	//cout << "a_0 = " << a_0.get() << "\na_1 = " << a_1.get() << "\no = " << o << "\nm = " << m
		//<< "\nz_1 = (a_1 + (a_0 << (o - m))) << ((m - o) / 2) = " << z_1.get() << endl;
	BigInt z_1 = a_1 << (m-o);
	//cout << "z_1 = a_1 << (m-o) = " << z_1.get() << endl;
	z_1  = z_1 + a_0;
	//cout << "z_1 + a_0 = " << z_1.get() << endl;
	z_1 = z_1 << ((o-m) / 2);
	//cout << "z_1 * 2^32^((o-m)/2) = " << z_1.get() << endl;
	z_1 = z_1 * z_1;
	//cout << "z_1^2 = " << z_1.get() << endl;

	z_1 = z_1 + ~(z_2 << (m-o));
	//cout << "z_2 = a_1^2 = " << z_2.get() << "\nz_1 - (z_2 * 2^32^(m-o) = " << z_1.get() << endl;
	z_1 = z_1 + ~(z_0 << (o-m));
	//cout << "z_0 = a_0^2 = " << z_0.get() << "\nz_1 - (z_0 * 2^32^(o-m) = " << z_1.get() << endl;
	z_1 = z_2 + z_1 + z_0;
	return z_1;
}

void BigInt::split(BigInt &a, BigInt &b) const {
	a.data.clear();
	b.data.clear();
	b.data.resize(data.size());
	int a_k = data.size() / 2;
	a.data.resize(a_k);
	for (int i = 0; i < a_k; i++) {
		a.data[i] = data[i];
	}
	for (int i = a_k; i < data.size(); i++) {
		b.data[i] = data[i];
	}
}

void BigInt::prune() {
	using std::cout; using std::endl;
	//std::cout << "x = " << x.get() << std::endl;
	//truncate();
	BigInt e = this->squared(64);
	//BigInt e = this->squared();
	//std::cout << "x * x = " << e.get() << endl;
	e <<= 1;
	//cout << "x * x * 2 = " << e.get() << endl;
	e = ~e;
	//cout << "x * x * 2 * -1 = " << e.get() << endl;
	e = e + 1;
	//cout << "1 - (x*x*2) = " << e.get() << endl << endl;
	//std::cout << "e = " << e.get() << std::endl;
	if ( e > 1) {
		e = ~e;
		//std::cout << "(x*x*2)+1 = " << e.get() << endl;
	}
	rmask(e);
	//std::cout << "x fixed = " << x.get() << std::endl;
	//truncate();
}

std::string BigInt::get() const {
	std::string s;
	int chars = 0;
	BigInt n = *this;
	if (n.data.size() > 0 && (n.data[0] & (1U << (BIGINT_SIZE - 1))) != 0) {
		//n = ~n;
		//s += "-";
	}
	for (int i = 0; i < n.data.size(); i++) {
		chunk t = n.data[i];
		for (int j = (BIGINT_SIZE / 4) - 1; j >= 0; j--) {
			unsigned char nyb = t >> (j * 4);
			nyb &= 0xF;
			if (i != 0 || nyb != 0 || j == 0) {
				if (i != 0) {
					if (chars % 8 == 0) {
						s += " ";
					}
					chars++;
				}
				s += intToHex(nyb);
			}
		}
		if (i == 0) {
			s += ".";
		}
	}
	if (n.data.size() == 1) {
		s += " 0";
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

std::ostream& operator<<(std::ostream &os, const BigInt &n) {
	os << n.get().c_str();
	return os;
}