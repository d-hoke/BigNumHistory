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
	for (int i = offset + get_global_id(0); i < size; i += get_global_size(0)) {
		//if (idx < size) {
			n[i] = pattern;
		//}
	}
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

// void carry(n, carry, size)
// carry holds the carry values of every 512 limbs
// expects num workgroups to be 1, and normal workgroup size to be 512
__kernel void carry(__global unsigned int *n, __global unsigned int *carry, int size) {
	const int idx = get_global_id(0);
	if (idx == 0) {
		for (int i = (size - (size % 512)) / 512 - 1; i > 0; i--) {
			int curCarry = carry[i];
			for (int curLimb = i * 512 - 1; curLimb > 0 && curCarry > 0; curLimb--) {
				unsigned int p = n[curLimb];
				n[curLimb] += curCarry;
				if (n[curLimb] < p) {
					curCarry = 1;
				} else {
					curCarry = 0;
				}
				if (curLimb % 512 == 0) {
					curCarry += carry[curLimb / 512];
					i--;
				}
			}
		}
	}
}

// void carry2(n, carry, size)
// carry holds the carry values of every limb
// expects num workgroups to be 1, and normal workgroup size to be 512
__kernel void carry2(__global unsigned int *n, __global unsigned int *carry, int size) {
	const int idx = get_global_id(0);
	if (idx == 0) {
		ulong curCarry = 0;
		for (int i = size - 1; i >= 0; i--) {
			curCarry += carry[i];
			if (curCarry != 0) {
				curCarry += n[i];
				n[i] = curCarry & 0xFFFFFFFF;
				curCarry >>= 32;
			}
		}
	}
}

// void add(n1, n2, carry, size)
// expects workgroup size to be 512
__kernel void add(__global unsigned int *n1, __global unsigned int *n2, __global unsigned int *carry, int size) {
	const int idx = get_global_id(0);
	const int localId = get_local_id(0);

	if (idx < size) {
		ulong t = n1[idx];
		t += n2[idx];
		n1[idx] = t & 0xFFFFFFFF;
		t >>= 32;
		if (idx > 0) {
			carry[idx - 1] = t;
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
				farCarry++;
			}
		}
		r[curDigit] = (uint)(curR & 0xFFFFFFFF);
		if (curDigit == 0) {
			//carry[sizeR - 1] = 0;
		} else if (curDigit > 0) {
			atomic_add(&carry[curDigit - 1], (curR >> 32));
			if (curDigit > 1) {
				atomic_add(&carry[curDigit - 2], farCarry);
			}
		}
	}
}

// void mul2(n1, n2, r, carry, size1, size2)
// perform only reads from n1 and n2, but each thread owns r[idx]
// expects workgroup size to be 512
__kernel void mul2(__global const unsigned int *n1, __global const unsigned int *n2,
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
				farCarry++;
			}
		}
		r[curDigit] = (uint)(curR & 0xFFFFFFFF);
		if (curDigit == 0) {
			//carry[sizeR - 1] = 0;
		} else if (curDigit > 0) {
			atomic_add(&carry[curDigit - 1], (curR >> 32));
			if (curDigit > 1) {
				atomic_add(&carry[curDigit - 2], farCarry);
			}
		}
	}
}

// void neg(n, carry, size)
// expects workgroup size to be 512
__kernel void neg(__global unsigned int *n, __global unsigned int *carry, int size) {
	__local int tmpCarry[512];
	const int idx = get_global_id(0);
	if (idx < size) {
		n[idx] = ~(n[idx]);
	}
	if (get_local_id(0) == 0) {
		carry[get_group_id(0)] = 0;
	}

	// condense carry
	barrier(CLK_LOCAL_MEM_FENCE);
	if (idx == (get_num_groups(0) - 1) * 512) {
		for (int i = 0; i < (size - 1) - idx; i++) {
			tmpCarry[i] = 0;
		}
		tmpCarry[size - 1 - idx] = 1;
		for (int i = 0; i < get_num_groups(0) - 1; i++) {
			carry[i] = 0;
		}
		int groupCarry = 0;
		condenseCarry(n, size, &groupCarry, idx, tmpCarry);
		carry[get_num_groups(0) - 1] = groupCarry;

		//carry[get_group_id(0)] = 1;
		//for (int i = size - 1; i >= idx; i--) {
		//	if (++n[i] != 0) { // not overflowing
		//		i = -1;
		//		carry[get_group_id(0)] = 0;
		//	}
		//}
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