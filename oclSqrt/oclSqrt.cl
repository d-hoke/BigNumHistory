#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

// void condenseCarry(n, sizeN, carry, startIdx, tmpCarry)
// expects you to make sure only 1 copy runs per 512
void condenseCarry(__global unsigned int *n, int sizeN, int *groupCarry, int startIdx, __local int *tmpCarry) {
	//carry[startIdx / 512] = 0;
	int i = (startIdx + 512 < sizeN ? startIdx + 512 : sizeN) - 1;
	for (; i >= startIdx; i--) {
		int offset = i - startIdx;
		if (tmpCarry[offset]) {
			unsigned int p = n[i];
			n[i] += tmpCarry[offset];
			if (n[i] < p) { // overflow
				if (offset > 0) {
					tmpCarry[offset - 1]++;
				} else {
					(*groupCarry)++;
				}
			}
		}
	}
}

// void fill(n, pattern, size)
__kernel void fill(__global unsigned int *n, int pattern, int offset, int size) {
	const int i = get_global_id(0) + offset;
	if (i < size) {
		n[i] = pattern;
	}

	//for (int i = offset + get_global_id(0); i < size; i += get_global_size(0)) {
	//	//if (idx < size) {
	//		n[i] = pattern;
	//	//}
	//}
}

// void shl(src, dst, size, d)
// expects BIGINT_LIMB_SIZE = 32, d > 0, dst initialized to 0
__kernel void shl(__global const unsigned int *src, __global unsigned int *dst, const int size, int d) {
	const int idx = get_global_id(0);
	const int indexedLimb = idx - (d / 32);
	const int bits = d % 32;

	if (idx < size) {
		if (indexedLimb >= 0) {
			atomic_or(&dst[indexedLimb], (src[idx] << bits));
			//dst[indexedLimb] |= (src[idx] << bits);
			
			if (indexedLimb > 0 && bits > 0) {
				atomic_or(&dst[indexedLimb - 1], (src[idx] >> (32 - bits)));
				//dst[indexedLimb - 1] |= (src[idx] >> (32 - bits));
			}
		}
	}
}

// void shr(src, dst, size, d)
// expects BIGINT_LIMB_SIZE = 32, d > 0, dst initialized to 0
__kernel void shr(__global const unsigned int *src, __global unsigned int *dst, const int size, int d) {
	const int idx = get_global_id(0);
	const int indexedLimb = idx + (d / 32);
	const int bits = d % 32;

	if (indexedLimb + (bits > 0 ? 1 : 0) < size) {
			atomic_or(&dst[indexedLimb], (src[idx] >> bits));
			//dst[indexedLimb] |= (src[idx] << bits);
			
		if (bits > 0) {
			atomic_or(&dst[indexedLimb + 1], (src[idx] << (32 - bits)));
				//dst[indexedLimb - 1] |= (src[idx] >> (32 - bits));
		}
	}
}

// void carry(n, carry, needCarry, size)
// carry holds the carry values of every limb
// expects one workitem for every limb
__kernel void carry(__global unsigned int *n, __global unsigned int *carry,
	__global bool *needCarry, int size) {
	const int idx = get_global_id(0);
	if (idx < size) {
		ulong curCarry = carry[idx];
		bool ok = true;
		for (int i = idx; i >= 0 && ok == true; i--) {
			if (curCarry == 0) {
				ok = false;
				carry[idx] = 0;
			} else if (i > 0 && carry[i - 1] != 0) {
				ok = false;
				carry[i] = curCarry;
				if (i != idx) carry[idx] = 0;
				*needCarry = true;
			} else {
				curCarry += n[i];
				n[i] = curCarry & 0xFFFFFFFF;
				curCarry >>= 32;
			}
		}
	}
}

// void carry2(n, carry, size)
// carry holds the carry values of every limb
// expects num workgroups to be 1
__kernel void carry2(__global unsigned int *n, __global unsigned int *carry, __global bool *needCarry,
	int size) {
	const int idx = get_global_id(0);
	if (idx == 0 && *needCarry == true) {
		ulong curCarry = 0;
		for (int i = size - 1; i >= 0; i--) {
			curCarry += carry[i];
			curCarry += n[i];
			n[i] = curCarry & 0xFFFFFFFF;
			curCarry >>= 32;
		}
	}
}

// void add(n1, n2, carry, size)
// expects workgroup size to be 512
__kernel void add(__global unsigned int *n1, __global unsigned int *n2, __global unsigned int *carry,
	int size) {
	const int idx = get_global_id(0);
	const int localId = get_local_id(0);

	if (idx < size) {
		ulong t = n1[idx];
		t += n2[idx];
		n1[idx] = t & 0xFFFFFFFFU;
		t >>= 32;
		if (idx > 0) {
			carry[idx - 1] = t;
			//if (t > 0) {
				//*needCarry = true;
			//}
		} else {
			carry[size - 1] = 0;
		}
	}
}

// void oldMul(n1, n2, r, carry, size1, size2)
// perform only reads from n1 and n2, but each thread owns r[idx] and carry[idx - 1]
// expects workgroup size to be 512
__kernel void oldMul(__global const unsigned int *n1, __global const unsigned int *n2,
	__global unsigned int *r, __global unsigned int *carry, int size1, int size2) {
	const int localId = get_local_id(0);
	const int curDigit = get_global_id(0);
	const int sizeR = size1 + size2 - 2 + 1;

	if(curDigit < sizeR) {
		ulong curR = 0;
		unsigned int largeSize = (size1 > size2 ? size1 : size2) - 1;
		int start = curDigit - largeSize;
		start = start < 0 ? 0 : start;
		int end = (curDigit < size2 - 1 ? curDigit + 1 : size2);
		for (int i = start; i < end; i++) {
			int multiplicand = curDigit - i;
			for (int curBit = 0; curBit < 32; curBit++) {
				if ((n2[i] & (1 << curBit)) != 0) {
					unsigned int addend = 0;
					addend |= (n1[multiplicand] << curBit);
					if (multiplicand + 1 < size1 && curBit != 0) {
						addend |= (n1[multiplicand + 1] >> (32 - curBit));
					}
					curR += addend;
				}
			}
		}
		if (curDigit < size2 - 1) {
			int multiplicand = -1;
			for (int curBit = 1; curBit < 32; curBit++) {
				if ((n2[curDigit + 1] & (1 << curBit)) != 0) {
					unsigned int addend = (n1[multiplicand + 1] >> (32 - curBit));
					curR += addend;
				}
			}
		}
		r[curDigit] = curR & 0xFFFFFFFF;
		if (curDigit > 0) {
			carry[curDigit - 1] = curR >> 32;
		} else {
			carry[sizeR - 1] = 0;
		}
	}
}

// void mul(n1, n2, r, carry, size1, size2)
// perform only reads from n1 and n2, but each thread owns r[idx]
// expects workgroup size to be 512
__kernel void mul(__global const unsigned int *n1, __global const unsigned int *n2,
	__global unsigned int *r, __global unsigned int *carry, int size1, int size2) {
	const int localId = get_local_id(0);
	const int curDigit = get_global_id(0);
	const int sizeR = size1 + size2 - 2 + 1;
	const int largeSize = (size2 > size1 ? size2 : size1) - 1;

	if(curDigit < sizeR) {
		int start = curDigit - largeSize;
		start = start < 0 ? 0 : start;
		int end = (curDigit < size2 - 1 ? curDigit + 1 : size2);
		int multiplicand = curDigit - start;
		ulong curR = 0;
		uint farCarry = 0; // if curR overflows
		for (int i = start; i < end && multiplicand < size1; i++, multiplicand = curDigit - i) {
			ulong p  = curR;
			curR += (ulong)n2[i] * n1[multiplicand];
			if (curR < p) {
				int pf = farCarry;
				farCarry++;
				if (farCarry < pf) {
					for (int j = 0; j < sizeR; j++) {
						r[j] = 404;
						carry[j] = 0;
					}
				}
			}
		}
		r[curDigit] = (uint)(curR & 0xFFFFFFFF);
		if (curDigit > 0) {
			atomic_add(&carry[curDigit - 1], (curR >> 32));
			if (curDigit > 1) {
				atomic_add(&carry[curDigit - 2], farCarry);
			}
		}
	}
}

// void mul2(n1, n2, r, carry, carry2, size1, size2)
// perform only reads from n1 and n2, but each thread owns r[idx]
// expects workgroup size to be 512
__kernel void mul2(__global const unsigned int *n1, __global const unsigned int *n2,
	__global unsigned int *r, __global unsigned int *carry, __global uint *carry2,
	int size1, int size2) {
	const int localId = get_local_id(0);
	const int curDigit = get_global_id(0);
	const int sizeR = size1 + size2 - 2 + 1;
	const int largeSize = (size2 > size1 ? size2 : size1) - 1;

	if(curDigit < sizeR) {
		int start = curDigit - largeSize;
		start = start < 0 ? 0 : start;
		int end = (curDigit < size2 - 1 ? curDigit + 1 : size2);
		int multiplicand = curDigit - start;
		ulong curR = 0;
		uint farCarry = 0; // if curR overflows
		for (int i = start; i < end && multiplicand < size1; i++, multiplicand = curDigit - i) {
			ulong p  = curR;
			curR += (ulong)n2[i] * n1[multiplicand];
			if (curR < p) {
				farCarry++;
			}
		}
		r[curDigit] = (uint)(curR & 0xFFFFFFFF);
		if (curDigit == 0) {
			//carry[sizeR - 1] = 0;
		} else if (curDigit > 0) {
			curR >>= 32;
			if (curR > 0) {
				carry[curDigit - 1] = curR;
				//*needCarry = true;
			}
			if (curDigit > 1 && farCarry > 0) {
				carry2[curDigit - 2] = farCarry;
				//*needCarry = true;
			}
		}
	}
}

// void neg(n, carry, size)
// expects workgroup size to be 512
__kernel void neg(__global unsigned int *n, int size) {
	const int idx = get_global_id(0);
	if (idx < size) {
		n[idx] = ~(n[idx]);
	}
}

// void carryOne(n, size)
// expects workgroup = 1
// used in setNeg, adds one to the very end and carries if necessary
__kernel void carryOne(__global unsigned int *n, int size) {
	const int idx = get_global_id(0);
	if (idx == 0) {
		bool ok = true;
		for (int i = size - 1; i >= 0 && ok == true; i--) {
			n[i]++;
			if (n[i] != 0) {
				ok = false;
			}
		}
	}
}

// void rmask(n, mask, sizeN, count)
__kernel void rmask(__global unsigned int *n, __global const unsigned int *mask, int sizeN, __global unsigned int *newSize) {
	const unsigned int idx = get_global_id(0);
	if (idx < sizeN && idx > 0) {
		if (mask[idx - 1] != 0) {
			n[idx - 1] = 0;
		} else if (mask[idx] != 0) {
			int bitsOk;
			bool isOk = true;
			for (bitsOk = 0; isOk == true; bitsOk++) {
				unsigned int x = 1U << (32 - bitsOk - 1);
				if ((mask[idx] & x) != 0) {
					isOk = false;
				}
			}
			if (bitsOk > 0) {
				n[idx] &= (~0 << (32 - bitsOk));
				*newSize = idx + 1;
			} else {
				*newSize = idx;
			}
		}
	}
}

// void count(n, sizeN, newSize)
__kernel void count(__global unsigned int *n, int sizeN, __global unsigned int *newSize) {
	if (get_global_id(0) == 0) {
		int i;
		bool ok = true;
		for (i = sizeN - 1; i >= 0 && ok == true; i--) {
			if (n[i] != 0) {
				ok = false;
				i++; // to make up for the coming i--
			}
		}
		*newSize = i + 1;
	}
}