/*
 * BigNum.h
 *
 *  Created on: Jun 22, 2012
 *      Author: peter
 */

#ifndef BIGNUM_H_
#define BIGNUM_H_

#include <map>
#include <stdint.h>
#include <cmath>

#define MAX_VAL 0xFFFFFFFF

typedef std::map<int, uint32_t> digitmap;

class BigNum {
	void setDigit(int _place, uint32_t _val) {
		digits[_place]
	}

	void carry(int _place) {
		int64_t newVal = digits[_place + 1] + 1;
		if (newVal > MAX_VAL) {
			carry(_place + 1);
		} else {
			digits[_place + 1] = newVal;
		}
	}

	// The problem is if I'm try to subtract 0.2 from 1000.1. I have to know what the maximum place is.
	void borrow(int _place) {
		digitmap::iterator it = digits.find(_place);
		if(it)
		int64_t newVal = digits[_place + 1] - 1;
		if (newVal)
	}

	int8_t isPositive = 1;
	digitmap digits;
public:
	BigNum();
	virtual ~BigNum();

	void set(double _val) {
		uint32_t whole = floor(_val);
		digits[0] = whole;
		digits[-1] = (uint32_t)((_val - whole) * MAX_VAL);
	}

	BigNum operator+(BigNum &_rhs) {
		BigNum retVal;
		// go through each digit
		retVal.isPositive = isPositive;
		for(digitmap::iterator itLhs = digits.begin(); itLhs != digits.end(); itLhs++) {
			retVal.digits[itLhs->first] = itLhs->second;
		}
		for(digitmap::iterator itRhs = _rhs.digits.begin(); itRhs != _rhs.digits.end(); itRhs++) {
			int64_t newVal = 0;
			if(retVal.digits.count(itRhs->first) != 0) { // if the num already exists
				newVal = retVal.digits[itRhs->first];
			}
			newVal += itRhs->second * isPositive * _rhs.isPositive; // add if both are same, subtract otherwise
			if (newVal > MAX_VAL) {
				retVal.carry(itRhs->first);
				newVal -= MAX_VAL - 1;
			} else if (newVal < 0) {
				retVal.borrow(itRhs);
				newVal += MAX_VAL + 1;
			}
			if (newVal == 0) {
				retVal.digits.erase(itRhs->first);
			}
		}
		return retVal;
	}
};

#endif /* BIGNUM_H_ */
