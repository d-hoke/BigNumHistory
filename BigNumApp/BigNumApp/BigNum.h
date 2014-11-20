#pragma once
#define VAL_TYPE unsigned long long
#define MAX_VAL 0x100000000
class BigNum
{
	void add(int _place, VAL_TYPE _val) {
		val[_place] += _val;
		if (_val[_place] > MAX_VAL) {
			VAL_TYPE addVal = val[_place] / MAX_VAL;
			val[_place] %= MAX_VAL;
			add(_place + 1, addVal);
		}
	}

	std::map<int, VAL_TYPE> val;
public:
	BigNum(void) { val[0] = 0; }
	BigNum(VAL_TYPE _val) { val[0] = _val; }
	~BigNum(void) { }
	//BigNum& operator+=(BigNum &_rhs) { val[0] += _rhs.val[0]; return *this; }
	BigNum& operator*=(BigNum &_rhs) {
		BigNum retVal;
		for(std::map<int, VAL_TYPE>::iterator lIter = val.begin(); lIter != val.end(); lIter++) {
			for(std::map<int, VAL_TYPE>::iterator rIter = _rhs.val.begin(); rIter != _rhs.val.end(); rIter++) {
				VAL_TYPE tVal = lIter->second * rIter->second;
				int place = lIter->first + rIter->first;
				add(place, tVal);
			}
		}
	}

	std::string getStr() {
		std::string retStr;
		char buffer[24];
		for(std::map<int, VAL_TYPE>::iterator iter = val.begin(); iter != val.end(); iter++) {
			_i64toa_s(iter->second, buffer, 24, 10);
			retStr += buffer;
			retStr += "*2e32e";
			_itoa_s(iter->first, buffer, 10);
			retStr += buffer;
		}
		return retStr;
	}
};

