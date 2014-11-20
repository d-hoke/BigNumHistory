/*
 * InfBinFraction.cpp
 *
 *  Created on: Nov 11, 2011
 *      Author: peter
 */

#include "InfBinFraction.h"
#include <cstdio>

InfBinFraction::~InfBinFraction() {
	// TODO Auto-generated destructor stub
}

void InfBinFraction::shiftPlace(lst::iterator _fullDigit) {
	if (_fullDigit->data > MAX_VAL) {
		if (_fullDigit == digits.begin()) {
			digits.push_front(digit(0, _fullDigit->place + 1));
		}
		lst::iterator prev = _fullDigit;
		(--prev)->data++;
		_fullDigit->data -= MAX_VAL;
		shiftPlace(prev);
	}
}

// sorted

InfBinFraction InfBinFraction::operator+(InfBinFraction &_rhs) {
	InfBinFraction retVal = *this;
	for (std::list<digit>::const_iterator iterRhs = _rhs.digits.begin(); iterRhs != _rhs.digits.end(); iterRhs++) {
		if (iterRhs->place > retVal.digits.front().place) {
			retVal.digits.push_front(*iterRhs);
		} else if (iterRhs->place < retVal.digits.back().place) {
			retVal.digits.push_back(*iterRhs);
		} else {
			lst::iterator iter = retVal.digits.begin();
			while (iterRhs->place < iter->place) {
				iter++;
			}
			if (iterRhs->place == iter->place) {
				iter->data += iterRhs->data;
				retVal.shiftPlace(iter);
			} else {
				retVal.digits.insert(iter, *iterRhs);
			}
		}
	}
	return retVal;
}

// TODO: Make subtraction

InfBinFraction InfBinFraction::operator-(InfBinFraction &_rhs) {
	InfBinFraction retVal = *this;
	for (std::list<digit>::const_iterator iterRhs = _rhs.digits.begin(); iterRhs != _rhs.digits.end(); iterRhs++) {
		if (iterRhs->place > retVal.digits.front().place) {
			retVal.digits.push_front(*iterRhs);
		} else if (iterRhs->place < retVal.digits.back().place) {
			retVal.digits.push_back(*iterRhs);
		} else {
			lst::iterator iter = retVal.digits.begin();
			while (iterRhs->place < iter->place) {
				iter++;
			}
			if (iterRhs->place == iter->place) {
				iter->data += iterRhs->data;
				retVal.shiftPlace(iter);
			} else {
				retVal.digits.insert(iter, *iterRhs);
			}
		}
	}
	return retVal;
}

std::ostream &operator<<(std::ostream &_stream, InfBinFraction _val) {
	for (std::list<digit>::iterator iter = _val.digits.begin(); iter != _val.digits.end();) {
		_stream << iter->data << "*2^(" << (32 * iter->place) << ")";
		if (++iter != _val.digits.end()) {
			_stream << " + ";
		}
	}
	return _stream;
}
