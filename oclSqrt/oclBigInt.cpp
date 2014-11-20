#include "StdAfx.h"
#include "oclBigInt.h"
#include "BigInt.h"

void oclBigInt::fillBuffer(cl_mem buffer, unsigned int pattern, unsigned int offset, unsigned int size) {
	cl_int error;
	error = clSetKernelArg(oclBigInt::fillKernel, 0, sizeof(cl_mem), &buffer);
	error |= clSetKernelArg(oclBigInt::fillKernel, 1, sizeof(unsigned int), &pattern);
	error |= clSetKernelArg(oclBigInt::fillKernel, 2, sizeof(unsigned int), &offset);
	error |= clSetKernelArg(oclBigInt::fillKernel, 3, sizeof(size_t), &size);
	if (error != CL_SUCCESS) {
		std::cout << "Error in fill : set fill args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	size_t workItems = getNumWorkItems(size);
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::fillKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in fill : enqueue fill limbs : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
}

size_t oclBigInt::getNumWorkItems(size_t numLimbs) {
	size_t workItems = numLimbs;
	if (numLimbs % oclBigInt::workItemsPerGroup != 0) {
		workItems += oclBigInt::workItemsPerGroup - (numLimbs % oclBigInt::workItemsPerGroup);
	}
	return workItems;
}

void oclBigInt::resize(const size_t size) {
	if (numLimbs == size) {
		return;
	}
	cl_int error;
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in resize : create buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	if (numLimbs > 0) {
		int copySize;
		if (numLimbs < size) {
			copySize = numLimbs * sizeof(unsigned int);
			fillBuffer(newLimbs, 0, numLimbs, size);
		} else {
			copySize = size * sizeof(unsigned int);
		}
		error = clEnqueueCopyBuffer(oclBigInt::queue, limbs, newLimbs, 0, 0, copySize, 0, NULL, NULL);
		if (error != CL_SUCCESS) {
			std::cout << "Error in resize : copy buffer : " << error << std::endl;
			std::cin.get();
			exit(error);
		}
		clReleaseMemObject(limbs);
	} else {
		fillBuffer(newLimbs, 0, 0, size);
	}
	numLimbs = size;
	limbs = newLimbs;
}

void oclBigInt::rmask(const oclBigInt &mask) {
	cl_int error;

	cl_mem newSize_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : create buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clSetKernelArg(oclBigInt::rmaskKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::rmaskKernel, 1, sizeof(cl_mem), &mask.limbs);
	error |= clSetKernelArg(oclBigInt::rmaskKernel, 2, sizeof(int), &numLimbs);
	error |= clSetKernelArg(oclBigInt::rmaskKernel, 3, sizeof(cl_mem), &newSize_d);
	if (error != CL_SUCCESS) {
		std::cout << "Error in rmask : set rmask args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	size_t workItems = oclBigInt::getNumWorkItems(numLimbs);
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::rmaskKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in rmask : enqueue rmask : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	int newSize = 0;
	error = clEnqueueReadBuffer(oclBigInt::queue, newSize_d, CL_TRUE, 0, sizeof(unsigned int), &newSize, 0, NULL, NULL);
	resize(newSize > 0 ? newSize : 1);
}

void oclBigInt::truncate() {
	cl_int error;

	cl_mem newSize_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : create buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clSetKernelArg(oclBigInt::countKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::countKernel, 1, sizeof(int), &numLimbs);
	error |= clSetKernelArg(oclBigInt::countKernel, 2, sizeof(cl_mem), &newSize_d);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : set count args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueTask(oclBigInt::queue, oclBigInt::countKernel, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : enqueue count : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	
	error = clFlush(oclBigInt::queue);
	if (error != CL_SUCCESS) {
		std::cout << "stop " << std::endl;
	}
	error = clFinish(oclBigInt::queue);
	if (error != CL_SUCCESS) {
		std::cout << "stop " << std::endl;
	}

	int newSize = 0;
	error = clEnqueueReadBuffer(oclBigInt::queue, newSize_d, CL_TRUE, 0, sizeof(unsigned int), &newSize, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : read count : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	resize(newSize > 0 ? newSize : 1);
}

oclBigInt &oclBigInt::oldMul(const oclBigInt &n) {
	cl_int error;
	
	size_t sizeR = numLimbs + n.numLimbs - 2 + 1;
	cl_mem r_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : create r buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	size_t workItems = oclBigInt::getNumWorkItems(sizeR);
	cl_mem carry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : create carry buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clSetKernelArg(oclBigInt::oldMulKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::oldMulKernel, 1, sizeof(cl_mem), &n.limbs);
	error |= clSetKernelArg(oclBigInt::oldMulKernel, 2, sizeof(cl_mem), &r_d);
	error |= clSetKernelArg(oclBigInt::oldMulKernel, 3, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::oldMulKernel, 4, sizeof(int), &numLimbs);
	error |= clSetKernelArg(oclBigInt::oldMulKernel, 5, sizeof(int), &n.numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : set mul args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::oldMulKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : enqueue mul : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	clReleaseMemObject(limbs);
	limbs = r_d;
	numLimbs = sizeR;

	error = clSetKernelArg(oclBigInt::carry2Kernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 1, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 2, sizeof(size_t), &numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : set carry args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueTask(oclBigInt::queue, oclBigInt::carry2Kernel, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : enqueue carry : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	clReleaseMemObject(carry_d);

	return *this;
}

oclBigInt &oclBigInt::baseMul(const oclBigInt &n) {
	cl_int error;
	
	size_t sizeR = numLimbs + n.numLimbs - 2 + 1;
	cl_mem r_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : create r buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	size_t workItems = oclBigInt::getNumWorkItems(sizeR);
	cl_mem carry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : create carry buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	fillBuffer(carry_d, 0, 0, sizeR);

	error = clSetKernelArg(oclBigInt::mulKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::mulKernel, 1, sizeof(cl_mem), &n.limbs);
	error |= clSetKernelArg(oclBigInt::mulKernel, 2, sizeof(cl_mem), &r_d);
	error |= clSetKernelArg(oclBigInt::mulKernel, 3, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::mulKernel, 4, sizeof(int), &numLimbs);
	error |= clSetKernelArg(oclBigInt::mulKernel, 5, sizeof(int), &n.numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : set mul args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::mulKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : enqueue mul : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	clReleaseMemObject(limbs);
	limbs = r_d;
	numLimbs = sizeR;

	//std::vector<unsigned int> r_h(sizeR);
	//clEnqueueReadBuffer(oclBigInt::queue, r_d, CL_TRUE, 0, sizeR * sizeof(unsigned int), &r_h[0],
	//	0, NULL, NULL);

	//std::vector<unsigned int> carry_h(sizeR);
	//clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_TRUE, 0, sizeR * sizeof(unsigned int), &carry_h[0],
	//	0, NULL, NULL);

	error = clSetKernelArg(oclBigInt::carry2Kernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 1, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 2, sizeof(size_t), &numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : set carry args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueTask(oclBigInt::queue, oclBigInt::carry2Kernel, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : enqueue carry : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	clReleaseMemObject(carry_d);

	return *this;
}

oclBigInt::oclBigInt(void) {
	numLimbs = 0;
}

oclBigInt::oclBigInt(unsigned int i, unsigned int pos) {
	cl_int error;
	numLimbs = pos + 1;
	std::vector<unsigned int> transfer(numLimbs, 0U);
	transfer[pos] = i;
	limbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, numLimbs * sizeof(unsigned int), &(transfer[0]), &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(unsigned int): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
}

oclBigInt::oclBigInt(double d, unsigned int prec) {
	std::vector<unsigned int> transfer;
	for (int i = 0; i < prec; i++) {
		unsigned int t = (unsigned int)(floor(d));
		transfer.push_back(t);
		d -= t;
		d *= pow(2.0, 32);
	}
	numLimbs = transfer.size();
	cl_int error;
	limbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, numLimbs * sizeof(unsigned int), &(transfer[0]), &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(double): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
}

oclBigInt::oclBigInt(BigInt &n) {
	numLimbs = n.numLimbs();

	cl_int error;
	limbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, numLimbs * sizeof(unsigned int), &(n.limbs[0]), &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(BigInt): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
}

oclBigInt::~oclBigInt(void)
{
	//std::cout << "release ocl" << std::endl;
	clReleaseMemObject(limbs);
}

bool oclBigInt::operator>(const int n) {
	cl_int error;

	cl_uint limb0;
	error = clEnqueueReadBuffer(oclBigInt::queue, limbs, CL_TRUE, 0, sizeof(unsigned int), &limb0, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error reading in > : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (limb0 > n || (limb0 == n && numLimbs > 1)) {
		return true;
	}
	return false;
}

oclBigInt &oclBigInt::operator<<=(const int d) {
	using std::cout; using std::endl;

	if (d < 0) {
		return *this >>= (d * -1);
	} else if (d == 0) {
		return *this;
	}

	cl_int error;
	size_t workItems = getNumWorkItems(numLimbs);

	cl_mem src_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_ONLY, sizeof(unsigned int) * numLimbs, NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : src_d create buffer : " << error << endl;
		std::cin.get();
		exit(error);
	}

	error = clFlush(oclBigInt::queue);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : before copy buffer : " << error << endl;
		std::cin.get();
		//exit(error);
	}

	error = clFinish(oclBigInt::queue);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : before copy buffer : " << error << endl;
		std::cin.get();
		//exit(error);
	}

	error = clEnqueueCopyBuffer(oclBigInt::queue, limbs, src_d, 0, 0, sizeof(unsigned int) * numLimbs, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : copy buffer from limbs to src_d : " << error << endl;
		std::cin.get();
		//exit(error);
	}

	fillBuffer(limbs, 0, 0, numLimbs);

	error = clSetKernelArg(oclBigInt::shlKernel, 0, sizeof(cl_mem), &src_d);
	error |= clSetKernelArg(oclBigInt::shlKernel, 1, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::shlKernel, 2, sizeof(size_t), &numLimbs);
	error |= clSetKernelArg(oclBigInt::shlKernel, 3, sizeof(int), &d);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : set shl args : " << error << endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shlKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : enqueue shl : " << error << endl;
		std::cin.get();
		exit(error);
	}

	clReleaseMemObject(src_d);

	return *this;
}

oclBigInt &oclBigInt::operator>>=(const int d) {
	using std::cout; using std::endl;

	if (d < 0) {
		return *this <<= (d * -1);
	} else if (d == 0) {
		return *this;
	}

	cl_int error;

	oclBigInt r;
	r.resize(numLimbs + d / 32 + (d % 32 > 0 ? 1 : 0));

	size_t workItems = getNumWorkItems(r.numLimbs);

	error = clSetKernelArg(oclBigInt::shrKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::shrKernel, 1, sizeof(cl_mem), &r.limbs);
	error |= clSetKernelArg(oclBigInt::shrKernel, 2, sizeof(size_t), &r.numLimbs);
	error |= clSetKernelArg(oclBigInt::shrKernel, 3, sizeof(int), &d);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : set shr args : " << error << endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shrKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : enqueue shr : " << error << endl;
		std::cin.get();
		exit(error);
	}

	clReleaseMemObject(limbs);
	numLimbs = r.numLimbs;
	limbs = r.limbs;
	clRetainMemObject(r.limbs);

	return *this;
}

oclBigInt &oclBigInt::operator+=(const oclBigInt &n) {
	int minWidth = n.numLimbs;
	if (n.numLimbs > numLimbs) {
		resize(n.numLimbs);
	}
	
	cl_int error;
	size_t workItems = oclBigInt::getNumWorkItems(minWidth);
	cl_mem carry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, minWidth * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in += : create carry buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clSetKernelArg(oclBigInt::addKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::addKernel, 1, sizeof(cl_mem), &n.limbs);
	error |= clSetKernelArg(oclBigInt::addKernel, 2, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::addKernel, 3, sizeof(int), &minWidth);
	if (error != CL_SUCCESS) {
		std::cout << "Error in += : set add args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//BigInt b = this->toBigInt();

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::addKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in += : enqueue add : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//b = this->toBigInt();

	//std::vector<unsigned int> carry_h(minWidth);
	//error = clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_TRUE, 0, minWidth * sizeof(unsigned int), &carry_h[0], 0, NULL, NULL);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in setNeg : enqueue carry : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	error = clSetKernelArg(oclBigInt::carry2Kernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 1, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 2, sizeof(size_t), &minWidth);
	if (error != CL_SUCCESS) {
		std::cout << "Error in += : set carry args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueTask(oclBigInt::queue, oclBigInt::carry2Kernel, 0, NULL, NULL);
		if (error != CL_SUCCESS) {
		std::cout << "Error in += : enqueue carry : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//b = this->toBigInt();

	clReleaseMemObject(carry_d);

	return *this;
}

oclBigInt &oclBigInt::operator-=(const oclBigInt &n) {
	oclBigInt t;
	n.copy(t);
	t.setNeg();
	return *this += t;
}

oclBigInt &oclBigInt::operator*=(const oclBigInt &n) {
	using std::cout; using std::endl;
	cl_int error;
	const int minSize = 0x30000;
	
	if (getNumLimbs() < minSize && n.getNumLimbs() < minSize) {
		this->baseMul(n);
		return *this;
	}

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	int p; // max num limbs / 2 rounded up
	if (n.getNumLimbs() > getNumLimbs()) {
		p = n.getNumLimbs() / 2 + n.getNumLimbs() % 2;
	} else {
		p = getNumLimbs() / 2 + getNumLimbs() % 2;
	}

	oclBigInt a_1, a_0, b_1, b_0;
	copy(a_1);
	fillBuffer(a_1.limbs, 0x0, p, getNumLimbs());
	a_1 >>= 32;
	a_1.truncate(); // a_1 = m_1 / x^p

	copy(a_0);
	fillBuffer(a_0.limbs, 0x0, 0, p);
	a_0 <<= (p * 32 - 32);
	a_0.truncate(); // a_0 = m_0 / x^p

	n.copy(b_1);
	fillBuffer(b_1.limbs, 0x0, p, getNumLimbs());
	b_1 >>= 32;
	b_1.truncate(); // b_1 = n_1 / x^p

	n.copy(b_0);
	fillBuffer(b_0.limbs, 0x0, 0, p);
	b_0 <<= (p * 32 - 32);
	b_0.truncate(); // b_0 = n_0 / x^p

	oclBigInt c_2;
	a_1.copy(c_2);
	c_2 *= b_1;     // c_2 = m_1 * n_1 / x^(2p)

	oclBigInt c_0;
	a_0.copy(c_0);
	c_0 *= b_0;     // c_0 = m_0 * n_0 / x^(2p)

	// *******
	// * c_1 *
	// *******

	oclBigInt c_1;
	a_1.copy(c_1);
	c_1 += a_0;

	oclBigInt t;
	b_1.copy(t);
	t += b_0;

	c_1 *= t;   // (m_1 + m_0)(n_1 + n_0) / x^(2p)
	c_1 -= c_2; // ((m_1 + m_0)(n_1 + n_0) - r_2) / x^(2p)
	c_1 -= c_0; // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) / x^(2p)

	c_2 <<= 2 * 32; // c_2 = m_1 * n_1 * x^(2-2p)
	c_0 >>= (2 * p - 2) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
	c_1 >>= (p - 2) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

	c_0 += c_1;
	c_0 += c_2;

	clReleaseMemObject(limbs);
	numLimbs = c_0.numLimbs;
	limbs = c_0.limbs;
	clRetainMemObject(c_0.limbs);

	return *this;
}

void oclBigInt::setNeg() {
	cl_int error;

	size_t workItems = oclBigInt::getNumWorkItems(numLimbs);
	cl_mem carry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, workItems / 512 * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : create carry buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	// invert
	error = clSetKernelArg(oclBigInt::negKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::negKernel, 1, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::negKernel, 2, sizeof(size_t), &numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : set invert args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//BigInt y = this->toBigInt();

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::negKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : enqueue neg : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//y = this->toBigInt();

	// carry
	error = clSetKernelArg(oclBigInt::carryKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::carryKernel, 1, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::carryKernel, 2, sizeof(size_t), &numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : set carry args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//std::vector<unsigned int> carry_h(workItems / 512);
	//error = clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_TRUE, 0, workItems / 512 * sizeof(unsigned int), &carry_h[0], 0, NULL, NULL);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in setNeg : enqueue carry : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	const size_t carryKernelSize = 1;
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::carryKernel, 1, 0, &carryKernelSize, &carryKernelSize, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : enqueue carry : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//y = this->toBigInt();


	clReleaseMemObject(carry_d);
}

void oclBigInt::copy(oclBigInt &n) const {
	cl_int error;

	if (n.numLimbs > 0) {
		error = clReleaseMemObject(n.limbs);
		if (error != CL_SUCCESS) {
			std::cout << "Error in copy : release mem object : " << error << std::endl;
			std::cin.get();
			exit(error);
		}
	}

	n.numLimbs = numLimbs;

	n.limbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, numLimbs * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in copy : create buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueCopyBuffer(oclBigInt::queue, limbs, n.limbs, 0, 0, numLimbs * sizeof(unsigned int), 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in copy : copy buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
}

BigInt oclBigInt::toBigInt() const {
	cl_int error = clFlush(oclBigInt::queue);
	if (error != CL_SUCCESS) {
		std::cout << "flush error : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clFinish(oclBigInt::queue);
	if (error != CL_SUCCESS) {
		std::cout << "finish error : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	return BigInt(limbs, numLimbs, oclBigInt::queue);
}

void oclBigInt::verify() {
	oclBigInt e;
	copy(e);
	//BigInt y = e.toBigInt();
	//y = y * y;
	e *= *this;
	//if (y == e.toBigInt()) {
	//} else {
	//	std::cout << "really stop" << std::endl;
	//}

	//y = e.toBigInt();
	//y <<= 1;
	e <<= 1;
	//if (y == e.toBigInt()) {
	//} else {
	//	std::cout << "really stop" << std::endl;
	//}

	//y = e.toBigInt();
	//y.setNeg();
	e.setNeg();
	//if (y == e.toBigInt()) {
	//} else {
	//	BigInt a = e.toBigInt();
	//	std::cout << "really stop" << std::endl;
	//}

	//y = e.toBigInt();
	e += oclBigInt(1U);
	//y += BigInt(1U);
	//if (y == e.toBigInt()) {
	//} else {
	//	BigInt a = e.toBigInt();
	//	std::cout << "really stop" << std::endl;
	//	y == e.toBigInt();
	//	//BigInt a = e.toBigInt();
	//}

	if (e > 1) {
		//BigInt y = e.toBigInt();
		//std::cout << y << std::endl;
		e.setNeg();
		//y.setNeg();
		//std::cout << (y == e.toBigInt()) << std::endl;
		//y = e.toBigInt();
	}

	rmask(e);
	//truncate();
}

oclBigInt &oclBigInt::mul2(const oclBigInt &n, int minSize) {
	using std::cout; using std::endl;
	cl_int error;
	//const int minSize = 0x4000;
	
	if (getNumLimbs() < minSize || n.getNumLimbs() < minSize) {
		this->baseMul(n);
		return *this;
	}

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	int p; // max num limbs / 2 rounded up
	if (n.getNumLimbs() > getNumLimbs()) {
		p = n.getNumLimbs() / 2 + n.getNumLimbs() % 2;
	} else {
		p = getNumLimbs() / 2 + getNumLimbs() % 2;
	}

	oclBigInt a_1, a_0, b_1, b_0;
	copy(a_1);
	fillBuffer(a_1.limbs, 0x0, p, getNumLimbs());
	a_1 >>= 32;
	a_1.truncate(); // a_1 = m_1 / x^p

	copy(a_0);
	fillBuffer(a_0.limbs, 0x0, 0, p);
	a_0 <<= (p * 32 - 32);
	a_0.truncate(); // a_0 = m_0 / x^p

	n.copy(b_1);
	fillBuffer(b_1.limbs, 0x0, p, getNumLimbs());
	b_1 >>= 32;
	b_1.truncate(); // b_1 = n_1 / x^p

	n.copy(b_0);
	fillBuffer(b_0.limbs, 0x0, 0, p);
	b_0 <<= (p * 32 - 32);
	b_0.truncate(); // b_0 = n_0 / x^p

	oclBigInt c_2;
	a_1.copy(c_2);
	c_2.mul2(b_1, minSize);     // c_2 = m_1 * n_1 / x^(2p)

	oclBigInt c_0;
	a_0.copy(c_0);
	c_0.mul2(b_0, minSize);     // c_0 = m_0 * n_0 / x^(2p)

	// *******
	// * c_1 *
	// *******

	oclBigInt c_1;
	a_1.copy(c_1);
	c_1 += a_0;

	oclBigInt t;
	b_1.copy(t);
	t += b_0;

	c_1.mul2(t, minSize);   // (m_1 + m_0)(n_1 + n_0) / x^(2p)
	c_1 -= c_2; // ((m_1 + m_0)(n_1 + n_0) - r_2) / x^(2p)
	c_1 -= c_0; // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) / x^(2p)

	c_2 <<= 2 * 32; // c_2 = m_1 * n_1 * x^(2-2p)
	c_0 >>= (2 * p - 2) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
	c_1 >>= (p - 2) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

	c_0 += c_1;
	c_0 += c_2;

	clReleaseMemObject(limbs);
	numLimbs = c_0.numLimbs;
	limbs = c_0.limbs;
	clRetainMemObject(c_0.limbs);

	return *this;


	//cl_int error;
	//
	//size_t sizeR = numLimbs + n.numLimbs - 2 + 1;
	//cl_mem r_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in * : create r buffer : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	//size_t workItems = oclBigInt::getNumWorkItems(sizeR);
	//cl_mem carry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in * : create carry buffer : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	//fillBuffer(carry_d, 0, 0, sizeR);

	//error = clSetKernelArg(oclBigInt::mul2Kernel, 0, sizeof(cl_mem), &limbs);
	//error |= clSetKernelArg(oclBigInt::mul2Kernel, 1, sizeof(cl_mem), &n.limbs);
	//error |= clSetKernelArg(oclBigInt::mul2Kernel, 2, sizeof(cl_mem), &r_d);
	//error |= clSetKernelArg(oclBigInt::mul2Kernel, 3, sizeof(cl_mem), &carry_d);
	//error |= clSetKernelArg(oclBigInt::mul2Kernel, 4, sizeof(int), &numLimbs);
	//error |= clSetKernelArg(oclBigInt::mul2Kernel, 5, sizeof(int), &n.numLimbs);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in * : set mul args : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	//error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::mul2Kernel, 1, 0, &workItems,
	//	&oclBigInt::workItemsPerGroup, 0, NULL, NULL);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in * : enqueue mul : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	//clReleaseMemObject(limbs);
	//limbs = r_d;
	//numLimbs = sizeR;

	//std::vector<unsigned int> r_h(sizeR);
	//clEnqueueReadBuffer(oclBigInt::queue, r_d, CL_TRUE, 0, sizeR * sizeof(unsigned int), &r_h[0],
	//	0, NULL, NULL);

	//std::vector<unsigned int> carry_h(sizeR);
	//clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_TRUE, 0, sizeR * sizeof(unsigned int), &carry_h[0],
	//	0, NULL, NULL);

	//error = clSetKernelArg(oclBigInt::carry2Kernel, 0, sizeof(cl_mem), &limbs);
	//error |= clSetKernelArg(oclBigInt::carry2Kernel, 1, sizeof(cl_mem), &carry_d);
	//error |= clSetKernelArg(oclBigInt::carry2Kernel, 2, sizeof(size_t), &numLimbs);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in * : set carry args : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	//error = clEnqueueTask(oclBigInt::queue, oclBigInt::carry2Kernel, 0, NULL, NULL);
	//if (error != CL_SUCCESS) {
	//	std::cout << "Error in * : enqueue carry : " << error << std::endl;
	//	std::cin.get();
	//	exit(error);
	//}

	//clReleaseMemObject(carry_d);

	//return *this;
}

std::ostream& operator<<(std::ostream &os, const oclBigInt &n) {
	os << n.toBigInt();
	return os;
}

cl_context oclBigInt::context;
cl_command_queue oclBigInt::queue;
cl_kernel oclBigInt::fillKernel;
cl_kernel oclBigInt::shlKernel;
cl_kernel oclBigInt::shrKernel;
cl_kernel oclBigInt::addKernel;
cl_kernel oclBigInt::carryKernel;
cl_kernel oclBigInt::negKernel;
cl_kernel oclBigInt::mulKernel;
cl_kernel oclBigInt::rmaskKernel;
cl_kernel oclBigInt::countKernel;
cl_kernel oclBigInt::mul2Kernel;
cl_kernel oclBigInt::carry2Kernel;
cl_kernel oclBigInt::oldMulKernel;