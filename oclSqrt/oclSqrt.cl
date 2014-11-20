#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

//#define DEBUG_CL
#define ERROR_OUT_OF_BOUNDS  -1
#define ERROR_BAD_PARAMETERS -2
#define ERROR_OVERFLOW       -3

#ifdef DEBUG_CL
#define ERROR(errorCode, error, ixError, szError) \
	if (*ixError < szError) { error[*ixError] = errorCode;  (*ixError)++; }
#endif

#ifdef DEBUG_CL
#define SET(n, ixN, val, szN, error, ixError, szError) if (ixN < szN) { n[ixN] = val; } \
	else { ERROR(ERROR_OUT_OF_BOUNDS, error, ixError, szError); }
#else
#define SET(n, ixN, val, szN, error, ixError, szError) n[ixN] = val;
#endif
	

//// void condenseCarry(n, sizeN, carry, startIdx, tmpCarry)
//// expects you to make sure only 1 copy runs per 512
//void condenseCarry(__global unsigned int *n, int sizeN, int *groupCarry, int startIdx,
//	__local int *tmpCarry) {
//	//carry[startIdx / 512] = 0;
//	int i = (startIdx + 512 < sizeN ? startIdx + 512 : sizeN) - 1;
//	for (; i >= startIdx; i--) {
//		int offset = i - startIdx;
//		if (tmpCarry[offset]) {
//			unsigned int p = n[i];
//			n[i] += tmpCarry[offset];
//			if (n[i] < p) { // overflow
//				if (offset > 0) {
//					tmpCarry[offset - 1]++;
//				} else {
//					(*groupCarry)++;
//				}
//			}
//		}
//	}
//}

// void fill(n, pattern, offset, szN)
// error checking not possible
__kernel void fill(__global uint *n, uint pattern, uint offset, uint szN) {
	const uint ixN = get_global_id(0) + offset;
	if (ixN < szN) {
		n[ixN] = pattern;
	}
}

// void shl(src, dst, size, d)
// expects BIGINT_LIMB_SIZE = 32, d > 0, dst initialized to 0
#ifdef DEBUG_CL
__kernel void shl(__global const uint *src, __global uint *dst, const uint szDst, int d,
		__global int *error, __global int *ixError, uint szError) {
	if (d <= 0) ERROR(ERROR_BAD_PARAMETERS, error, ixError, szError);
#else
__kernel void shl(__global const uint *src, __global uint *dst, const uint szDst, int d) {
#endif
	const int ixDst = get_global_id(0);
	const int ixSrc = ixDst + (d / 32);
	const int bits = d % 32;
	const int szSrc = szDst + d / 32;

	if (ixSrc < szSrc) {
#ifdef DEBUG_CL
		if (ixDst >= szDst || ixSrc >= szSrc || ixDst < 0 || ixSrc < 0) {
			ERROR(ERROR_OUT_OF_BOUNDS, error, ixError, szError);
		}
#endif

		atomic_or(&dst[ixDst], (src[ixSrc] << bits));
		if (ixDst > 0 && bits > 0) {
#ifdef DEBUG_CL
			if ((ixDst - 1) >= szDst || ixSrc >= szSrc || ixSrc < 0 || (ixDst - 1) < 0) {
				ERROR(ERROR_OUT_OF_BOUNDS, error, ixError, szError);
			}
#endif
			atomic_or(&dst[ixDst - 1], (src[ixSrc] >> (32 - bits)));
		}
	}
}

// void shr(src, dst, szN, d)
// expects BIGINT_LIMB_SIZE = 32, d > 0, dst initialized to 0
#ifdef DEBUG_CL
__kernel void shr(__global const uint *src, __global uint *dst, const uint szDst,
	int d, __global int *error, __global int *ixError, uint szError) {
#else
__kernel void shr(__global const uint *src, __global uint *dst, const uint szDst, int d) {
#endif
	const int ixSrc = get_global_id(0);
	const int ixDst = ixSrc + (d / 32);
	const int bits = d % 32;
#ifdef DEBUG_CL
	const int szSrc = szDst - d / 32 - (bits > 0 ? 1 : 0);
	if (d <= 0) {
		ERROR(ERROR_BAD_PARAMETERS, error, ixError, szError);
	}
#endif

	if (ixDst + (bits > 0 ? 1 : 0) < szDst) {
#ifdef DEBUG_CL
		if (ixDst >= szDst || ixSrc >= szSrc || ixSrc < 0 || ixDst < 0) {
			ERROR(ERROR_OUT_OF_BOUNDS, error, ixError, szError);
		}
#endif
		atomic_or(&dst[ixDst], (src[ixSrc] >> bits));
			//dst[indexedLimb] |= (src[idx] << bits);
			
		if (bits > 0) {
#ifdef DEBUG_CL
			if ((ixDst + 1) >= szDst) ERROR(ERROR_OUT_OF_BOUNDS, error, ixError, szError);
			if (ixSrc >= szSrc) ERROR(ERROR_OUT_OF_BOUNDS, error, ixError, szError);
#endif
			atomic_or(&dst[ixDst + 1], (src[ixSrc] << (32 - bits)));
				//dst[indexedLimb - 1] |= (src[idx] >> (32 - bits));
		}
	}
}

//// old carry
//void shortCarry(__global unsigned int *n, volatile __global unsigned int *carry_d,
//	__local bool *needCarry_g, int size) {
//	const int idx = get_global_id(0);
//	if (idx < size) {
//		ulong curCarry = carry_d[idx];
//		bool ok = true;
//		for (int i = idx; i >= 0 && ok == true; i--) {
//			if (curCarry == 0) {
//				ok = false;
//				carry_d[idx] = 0; // possibly redundant
//			} else {
//				uint pCarry = curCarry;
//				curCarry += n[i];
//				// overflow and blocked
//				if (i > 0 && curCarry > 0xFFFFFFFFUL && carry_d[i-1] != 0) {
//					carry_d[i] = pCarry;
//					if (idx != i) carry_d[idx] = 0;
//					*needCarry_g = true;
//					ok = false;
//
//					//while(carry_d[i-1] != 0) {
//					//}
//				} else {
//					n[i] = curCarry & 0xFFFFFFFF;
//					curCarry >>= 32;
//				}
//				if (i == 0) {
//					carry_d[idx] = 0;
//					//carry_d[i] = 0;
//				}
//			}
//		}
//	}
//}

//// old carry
//// void carry(n, carry, needCarry, size)
//// carry holds the carry values of every limb
//// expects one workitem for every limb
//__kernel void carry(__global unsigned int *n, __global unsigned int *carry_d,
//	__global bool *needCarry, int size) {
//	const int idx = get_global_id(0);
//	__local bool needCarry_g;
//	shortCarry(n, carry_d, &needCarry_g, size);
//	//if (get_local_id(0) == 0 && needCarry_g) {
//	if (needCarry_g) {
//		*needCarry = true;
//	}
//	// make it so that within workgroups can resolve conflicts
//}

__kernel void checkAdd(__global unsigned int *n, __global unsigned int *carry_d,
	__global bool *needCarry, int szN, int offset) {
	const int idx = get_global_id(0);
	if (idx + offset < szN) {
		ulong curCarry = carry_d[idx + offset];
		bool ok = true;
		for (int i = idx; i >= 0 && ok == true; i--) {
			if (curCarry == 0) {
				ok = false;
				carry_d[idx + offset] = 0; // possibly redundant
			} else {
				uint pCarry = curCarry;
				curCarry += n[i];
				// overflow and blocked
				if (i > 0 && curCarry > 0xFFFFFFFFUL && carry_d[i - 1 + offset] != 0) {
					carry_d[i + offset] = pCarry;
					if (idx != i) carry_d[idx + offset] = 0;
					//*needCarry_g = true;
					*needCarry = true;
					ok = false;
				} else {
					n[i] = curCarry & 0xFFFFFFFF;
					curCarry >>= 32;
				}
				if (i == 0) carry_d[idx + offset] = 0;
			}
		}
	}
}

__kernel void shortAddOff(__global unsigned int *n, const __global unsigned int *n2,
	__global bool *needAdd, int szN2, int offset) {
	const int idx = get_global_id(0);
	if (idx == 0 && *needAdd == true) {
		ulong curCarry = 0;
		for (int i = szN2 - 1 - offset; i >= 0; i--) {
			curCarry += n2[i + offset];
			curCarry += n[i];
			n[i] = curCarry & 0xFFFFFFFF;
			//n2[i + offset] = 0; // DEBUG
			curCarry >>= 32;
		}
	}

	//if (*needAdd == false) return;

	//const int workgroupSize = 512;
	//__local uint carry_g[512];
	//__local int ltszN2;

	//if (*needCarry == false) {
	//	return;
	//}

	//if (idx < workgroupSize) {
	//	carry_g[idx] = 0;
	//}

	//int tszN2 = ltszN2;
	//while (tszN2 > workgroupSize) {
	//	const int width = (tszN2 + workgroupSize - 1 - offset) / workgroupSize; // rounded up
	//	if (idx < workgroupSize) {
	//		carry_g[idx] = 0;
	//		const int start = idx * width;
	//		if (start < tszN2) {
	//			int end = start + width;
	//			end = end > tszN2 ? tszN2 : end;
	//			ulong curCarry = 0;
	//			for (int i = end - 1; i >= start; i--) {
	//				curCarry += n2[i + offset];
	//				curCarry += n[i];
	//				n[i] = curCarry & 0xFFFFFFFF;
	//				n2[i + offset] = 0;
	//				curCarry >>= 32;
	//			}
	//			carry_g[idx] = curCarry;
	//		}
	//	}

	//	barrier(CLK_LOCAL_MEM_FENCE);
	//	if (idx > 0 && idx < workgroupSize && idx * width <= size) {
	//		carry[idx * width - 1] = carry_g[idx];
	//	} else if (idx == 0) {
	//		int newSize;
	//		bool ok = true;
	//		for (newSize = workgroupSize; newSize > 0 && ok == true; newSize--) {
	//			if (carry_g[newSize - 1] != 0) {
	//				ok = false;
	//			}
	//		}
	//		tSize = newSize * width;
	//	}
	//	barrier(CLK_LOCAL_MEM_FENCE);
	//	tSize_l = tSize;
	//}
	//
	//if (idx == 0 && tSize_l > 0) {
	//	ulong curCarry = 0;
	//	for (int i = tSize_l - 1; i >= 0; i--) {
	//		curCarry += carry[i];
	//		curCarry += n[i];
	//		n[i] = curCarry & 0xFFFFFFFF;
	//		curCarry >>= 32;
	//	}
	//}
}

// void carry2(n, carry, size)
// carry holds the carry values of every limb
// expects num workgroups to be 1, and workitems to be 512
//__kernel void carry2(__global unsigned int *n, __global unsigned int *carry, __global bool *needCarry,
//	int size) {
//	if (*needCarry) {
//		shortAddOff(n, carry, size, 1);
//	}

	//const int workgroupSize = 512;
	//__local uint carry_g[512];
	//__local int tSize;

	//if (*needCarry == false) {
	//	return;
	//}

	//if (idx < workgroupSize) {
	//	carry_g[idx] = 0;
	//}

	//int tSize_l = size;
	//while (tSize_l > workgroupSize) {
	//	const int width = tSize_l / workgroupSize + (tSize_l % workgroupSize == 0 ? 0 : 1);
	//	//for (int idx = 0; idx < workgroupSize; idx++) {
	//	if (idx < workgroupSize) {
	//		carry_g[idx] = 0;
	//		const int start = idx * width;
	//		if (start < tSize_l) {
	//			int end = start + width;
	//			end = end > tSize_l ? tSize_l : end;
	//			ulong curCarry = 0;
	//			for (int i = end - 1; i >= start; i--) {
	//				curCarry += carry[i];
	//				curCarry += n[i];
	//				n[i] = curCarry & 0xFFFFFFFF;
	//				carry[i] = 0;
	//				curCarry >>= 32;
	//			}
	//			carry_g[idx] = curCarry;
	//		}
	//	}
	//	//}

	//	barrier(CLK_LOCAL_MEM_FENCE);
	//	if (idx > 0 && idx < workgroupSize && idx * width <= size) {
	//		carry[idx * width - 1] = carry_g[idx];
	//	} else if (idx == 0) {
	//		int newSize;
	//		bool ok = true;
	//		for (newSize = workgroupSize; newSize > 0 && ok == true; newSize--) {
	//			if (carry_g[newSize - 1] != 0) {
	//				ok = false;
	//			}
	//		}
	//		tSize = newSize * width;
	//	}
	//	barrier(CLK_LOCAL_MEM_FENCE);
	//	tSize_l = tSize;
	//}
	//
	//if (idx == 0 && tSize_l > 0) {
	//	ulong curCarry = 0;
	//	for (int i = tSize_l - 1; i >= 0; i--) {
	//		curCarry += carry[i];
	//		curCarry += n[i];
	//		n[i] = curCarry & 0xFFFFFFFF;
	//		curCarry >>= 32;
	//	}
	//}
//}

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
		carry[idx] = t;
	}
}

void condenseCarry(__global uint *r, int sizeR, __local bool *lNeedCarry, __local uint *carry, int offset) {
	const int curDigit = get_global_id(0);
	const int localId = get_local_id(0);

	__local uint carryOverflow[512];

	ulong curR = 0;
	if (curDigit < sizeR && localId - offset >= 0) curR = r[curDigit - offset];
	while (*lNeedCarry == true) {
		if (curDigit < sizeR) {
			curR += carry[localId];
		}
		if (localId > 0) {
			carryOverflow[localId - 1] = (curR >> 32);
		}
		curR &= 0xFFFFFFFF;
		barrier(CLK_LOCAL_MEM_FENCE);
		if (localId == 0) {
			*lNeedCarry = false;
			for (int i = 0; i < 512 && curDigit + i < sizeR; i++) {
				if (i < offset) {
					carry[i] += carryOverflow[i];
				} else {
					carry[i] = carryOverflow[i];
					if (carry[i] > 0) {
						*lNeedCarry = true;
					}
				}
			}
			if (curDigit + 511 < sizeR) {
				carry[511] = 0;
			}
		}
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	//if (curDigit < sizeR) {
	//	r[curDigit] = 404;
	//}
	barrier(CLK_LOCAL_MEM_FENCE);
	if (curDigit < sizeR && localId - offset >= 0) {
		r[curDigit - offset] = curR;
	}
	//if (curDigit + 1 < sizeR && localId - offset >= 0) {
	//	r[curDigit - offset] = curR;
	//}
	//if (localId < 511 && curDigit + 1 < sizeR) {
	//	r[curDigit] = curR;
	//}
}

// void mul2(n1, n2, r, carry, carry2, size1, size2)
// perform only reads from n1 and n2, but each thread owns r[idx]
// expects workgroup size to be 512
__kernel void mul2(__constant unsigned int *n1, __constant unsigned int *n2,
		__global unsigned int *r, __global unsigned int *carry,
#ifdef DEBUG_CL
		int sz_n1, int sz_n2,
		__global int *error, __global int *ixError, uint szError) {
#else
		int sz_n1, int sz_n2) {
#endif
	const int localId = get_local_id(0);
	const int curDigit = get_global_id(0);
	const int sizeR = sz_n1 + sz_n2 - 2 + 1;
	const int largeSize = (sz_n2 > sz_n1 ? sz_n2 : sz_n1) - 1;
	__local uint ltCarry[512];
	__local bool lNeedCarry;
	__local uint ltCarry2[512];
	__local bool lNeedCarry2;

	if(curDigit < sizeR) {
		int start = curDigit + 1 - sz_n1;
		start = start < 0 ? 0 : start;
		int end = (curDigit < sz_n2 - 1 ? curDigit + 1 : sz_n2);
		int multiplicand = curDigit - start;
		ulong curR = 0;
		uint farCarry = 0; // if curR overflows
		for (int i = start; i < end; i++, multiplicand = curDigit - i) {
			ulong p  = curR;
			curR += (ulong)n2[i] * n1[multiplicand]; // access of n2 and n1
			if (curR < p) {
				farCarry++;
			}
		}

		r[curDigit] = (uint)(curR & 0xFFFFFFFF); // access of r
		if (curDigit >= 0) {
			curR >>= 32;
			ltCarry[localId] = curR;
			if (curR > 0) {
				lNeedCarry = true;
			}
			if (curDigit > 0) {
				//carry2[curDigit - 1] = farCarry;
				ltCarry2[localId] = farCarry;
				if (farCarry > 0) {
					lNeedCarry2 = true;
				}
			}
		}
	}
	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

	condenseCarry(r, sizeR, &lNeedCarry2, &ltCarry2, 2); // access of r

	if (curDigit + 1 < sizeR && localId < 511) {
#ifdef DEBUG_CL
		if (ltCarry[localId] + ltCarry2[localId + 1] < ltCarry[localId]) {
			ERROR(ERROR_OVERFLOW, error, ixError, szError);
		}
#endif
		ltCarry[localId] += ltCarry2[localId + 1];
	}

	condenseCarry(r, sizeR, &lNeedCarry, &ltCarry, 1); // access of r

	if (localId == 0 && curDigit < sizeR) {
		carry[curDigit] = ltCarry[localId];
	}

	if (localId == 0 && curDigit > 0) {
		carry[curDigit - 1] = ltCarry2[localId];
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

// void rmask(n, mask, sizeN, newSize)
__kernel void rmask(__global uint *n, __global const uint *mask, int szN, __global uint *newSize) {
	const uint idx = get_global_id(0);
	const int szMask = szN;
	if (idx == 0) {
		int ixMask;
		for (ixMask = 0; ixMask < szMask && mask[ixMask] == 0; ixMask++) ;

		int bitsOk;
		bool ok = true;
		for (bitsOk = 0; ok == true; bitsOk++) {
			uint x = 1U << (32 - bitsOk - 1);
			if ((mask[ixMask] & x) != 0) {
				ok = false;
			}
		}
		bitsOk--; // to compensate for final inc
		if (bitsOk > 0) {
			n[ixMask] &= (~0 << (32 - bitsOk));
			*newSize = ixMask + 1;
		} else {
			*newSize = ixMask;
		}
	}
					
	//if (idx < sizeN && idx > 0) {
	//	if (mask[idx - 1] != 0) {
	//		n[idx - 1] = 0;
	//	} else if (mask[idx] != 0) {
	//		int bitsOk;
	//		bool isOk = true;
	//		for (bitsOk = 0; isOk == true; bitsOk++) {
	//			unsigned int x = 1U << (32 - bitsOk - 1);
	//			if ((mask[idx] & x) != 0) {
	//				isOk = false;
	//			}
	//		}
	//		if (bitsOk > 0) {
	//			n[idx] &= (~0 << (32 - bitsOk));
	//			*newSize = idx + 1;
	//		} else {
	//			*newSize = idx;
	//		}
	//	}
	//}
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

__kernel void toggle(__global uint *n, int szN, __global uint *carry) {
	const int ixN = get_global_id(0);

	if (ixN < szN) {
		n[ixN] = n[ixN] * carry[0];
	}
}