#include "StdAfx.h"
#include "oclBigInt.h"
#include "BigInt.h"

void oclBigInt::fillBuffer(cl_mem buffer, unsigned int pattern, unsigned int offset, unsigned int size) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

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
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::fillKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in fill : enqueue fill limbs : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_fill += (eT - sT);
	}
	clReleaseEvent(event_profile);
}

size_t oclBigInt::getNumWorkItems(size_t numLimbs) {
	size_t workItems = numLimbs;
	if (numLimbs % oclBigInt::workItemsPerGroup != 0) {
		workItems += oclBigInt::workItemsPerGroup - (numLimbs % oclBigInt::workItemsPerGroup);
	}
	return workItems;
}

void oclBigInt::setLimbs(cl_mem newLimbs, cl_mem newCarry, size_t newSize) {
	if (numLimbs > 0) {
		clReleaseMemObject(limbs);
		clReleaseMemObject(carry_d);
	}
	limbs = newLimbs;
	numLimbs = newSize;
	carry_d = newCarry;
}

void oclBigInt::resize(const size_t size) {
	if (numLimbs == size) {
		return;
	}
	cl_int error;
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(unsigned int), 
		NULL, &error);
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
	} else {
		fillBuffer(newLimbs, 0, 0, size);
	}
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(cl_uint),
		NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in resize : create carry buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	setLimbs(newLimbs, newCarry, size);
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
	cl_event event_profile;
	cl_ulong sT, eT;

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

	error = clEnqueueTask(oclBigInt::queue, oclBigInt::countKernel, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : enqueue count : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_count += (eT - sT);
	}
	clReleaseEvent(event_profile);
	
	int newSize = 0;
	error = clEnqueueReadBuffer(oclBigInt::queue, newSize_d, CL_TRUE, 0, sizeof(unsigned int), &newSize, 0, NULL, NULL);
	if (error != CL_SUCCESS) {
		std::cout << "Error in truncate : read count : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	resize(newSize > 0 ? newSize : 1);

	clReleaseMemObject(newSize_d);
}

//oclBigInt &oclBigInt::oldMul(const oclBigInt &n) {
//	cl_int error;
//	
//	size_t sizeR = numLimbs + n.numLimbs - 2 + 1;
//	cl_mem r_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
//	if (error != CL_SUCCESS) {
//		std::cout << "Error in * : create r buffer : " << error << std::endl;
//		std::cin.get();
//		exit(error);
//	}
//
//	size_t workItems = oclBigInt::getNumWorkItems(sizeR);
//	cl_mem carry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeR * sizeof(unsigned int), NULL, &error);
//	if (error != CL_SUCCESS) {
//		std::cout << "Error in * : create carry buffer : " << error << std::endl;
//		std::cin.get();
//		exit(error);
//	}
//
//	error = clSetKernelArg(oclBigInt::oldMulKernel, 0, sizeof(cl_mem), &limbs);
//	error |= clSetKernelArg(oclBigInt::oldMulKernel, 1, sizeof(cl_mem), &n.limbs);
//	error |= clSetKernelArg(oclBigInt::oldMulKernel, 2, sizeof(cl_mem), &r_d);
//	error |= clSetKernelArg(oclBigInt::oldMulKernel, 3, sizeof(cl_mem), &carry_d);
//	error |= clSetKernelArg(oclBigInt::oldMulKernel, 4, sizeof(int), &numLimbs);
//	error |= clSetKernelArg(oclBigInt::oldMulKernel, 5, sizeof(int), &n.numLimbs);
//	if (error != CL_SUCCESS) {
//		std::cout << "Error in * : set mul args : " << error << std::endl;
//		std::cin.get();
//		exit(error);
//	}
//
//	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::oldMulKernel, 1, 0, &workItems, &oclBigInt::workItemsPerGroup, 0, NULL, NULL);
//	if (error != CL_SUCCESS) {
//		std::cout << "Error in * : enqueue mul : " << error << std::endl;
//		std::cin.get();
//		exit(error);
//	}
//
//	clReleaseMemObject(limbs);
//	limbs = r_d;
//	numLimbs = sizeR;
//
//	error = clSetKernelArg(oclBigInt::carry2Kernel, 0, sizeof(cl_mem), &limbs);
//	error |= clSetKernelArg(oclBigInt::carry2Kernel, 1, sizeof(cl_mem), &carry_d);
//	error |= clSetKernelArg(oclBigInt::carry2Kernel, 2, sizeof(size_t), &numLimbs);
//	if (error != CL_SUCCESS) {
//		std::cout << "Error in * : set carry args : " << error << std::endl;
//		std::cin.get();
//		exit(error);
//	}
//
//	error = clEnqueueTask(oclBigInt::queue, oclBigInt::carry2Kernel, 0, NULL, NULL);
//	if (error != CL_SUCCESS) {
//		std::cout << "Error in * : enqueue carry : " << error << std::endl;
//		std::cin.get();
//		exit(error);
//	}
//
//	clReleaseMemObject(carry_d);
//
//	return *this;
//}

oclBigInt &oclBigInt::baseMul(const oclBigInt &n) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

	oclBigInt r(numLimbs + n.numLimbs - 2 + 1);
	fillBuffer(r.carry_d, 0, 0, r.getNumLimbs());

	cl_mem carry2_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, r.getNumLimbs() * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : create carry2 buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	fillBuffer(carry2_d, 0, 0, r.getNumLimbs());

	error = clSetKernelArg(oclBigInt::mul2Kernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::mul2Kernel, 1, sizeof(cl_mem), &n.limbs);
	error |= clSetKernelArg(oclBigInt::mul2Kernel, 2, sizeof(cl_mem), &r.limbs);
	error |= clSetKernelArg(oclBigInt::mul2Kernel, 3, sizeof(cl_mem), &r.carry_d);
	error |= clSetKernelArg(oclBigInt::mul2Kernel, 4, sizeof(cl_mem), &carry2_d);
	error |= clSetKernelArg(oclBigInt::mul2Kernel, 5, sizeof(int), &numLimbs);
	error |= clSetKernelArg(oclBigInt::mul2Kernel, 6, sizeof(int), &n.numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : set mul args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	size_t workItems = oclBigInt::getNumWorkItems(r.getNumLimbs());
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::mul2Kernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : enqueue mul : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_baseMul += (eT - sT);
	}
	clReleaseEvent(event_profile);
	//setLimbs(r_d, carry_d, sizeR);
	move(r);

	//cl_mem needCarry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeof(bool), NULL,
	//		0);

	//BigInt check = this->toBigInt();
	//std::vector<cl_uint> c_h(getNumLimbs());
	//std::vector<cl_uint> c2_h(getNumLimbs());
	//clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_FALSE, 0, getNumLimbs() * sizeof(cl_uint), &c_h[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, carry2_d, CL_TRUE, 0, getNumLimbs() * sizeof(cl_uint), &c2_h[0],
	//	0, NULL, NULL);

	//fillBuffer(limbs, 0, 0, getNumLimbs() * sizeof(cl_uint));


	addCarry(carry_d, carry2_d, getNumLimbs(), oclBigInt::time_baseMul);
	//clReleaseMemObject(limbs);
	//limbs = carry_d;
	//carry_d = carry2_d;
	carry(carry_d, carry2_d, getNumLimbs());
	//carry(carry2_d, carry_d getNumLimbs());

	//BigInt check = this->toBigInt();

	//clReleaseMemObject(carry_d);
	clReleaseMemObject(carry2_d);
	//clReleaseMemObject(needCarry_d);

	return *this;

}

void oclBigInt::carry(cl_mem &carry_d, cl_mem &n2_d, int minWidth) {
	cl_event event_profile;
	cl_ulong sT, eT;
	cl_int error;

	size_t workItems = getNumWorkItems(minWidth);

	int iterations;
	for (iterations = 0; iterations < 2; iterations++) {
		cl_mem swap = n2_d;
		n2_d = carry_d;
		carry_d = swap;
		fillBuffer(carry_d, 0, 0, minWidth);

		error = clSetKernelArg(oclBigInt::addKernel, 0, sizeof(cl_mem), &limbs);
		error |= clSetKernelArg(oclBigInt::addKernel, 1, sizeof(cl_mem), &n2_d);
		error |= clSetKernelArg(oclBigInt::addKernel, 2, sizeof(cl_mem), &carry_d);
		error |= clSetKernelArg(oclBigInt::addKernel, 3, sizeof(int), &minWidth);
		if (error != CL_SUCCESS) {
			std::cout << "Error in carry : set add-carry args : " << error << std::endl;
			std::cin.get();
			exit(error);
		}

		error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::addKernel, 1, 0, &workItems,
			&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
		if (error != CL_SUCCESS) {
			std::cout << "Error in += : enqueue add-carry : " << error << std::endl;
			std::cin.get();
			exit(error);
		}

		if (oclBigInt::profile == true) {
			clFlush(oclBigInt::queue);
			clFinish(oclBigInt::queue);
			clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
			clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
			oclBigInt::time_add += (eT - sT);
		}
		clReleaseEvent(event_profile);
	}

	//clReleaseMemObject(n2_d);

	addCarry(limbs, carry_d, minWidth, oclBigInt::time_baseMul);
}

void oclBigInt::addCarry(cl_mem &buffer, cl_mem &carry_d, int minWidth, cl_ulong &timer) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;
	size_t workItems = getNumWorkItems(minWidth);

	//std::vector<cl_uint> c_h(getNumLimbs());
	//std::vector<cl_uint> c2_h(getNumLimbs());
	//clEnqueueReadBuffer(oclBigInt::queue, buffer, CL_FALSE, 0, getNumLimbs() * sizeof(cl_uint), &c_h[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_TRUE, 0, getNumLimbs() * sizeof(cl_uint), &c2_h[0],
	//	0, NULL, NULL);

	//for (int i = 0; i < 1; i++) {
		bool needCarry_h = false;
		error = clEnqueueWriteBuffer(oclBigInt::queue, oclBigInt::needCarry_d, CL_FALSE, 0, sizeof(bool),
			&needCarry_h, 0, NULL, NULL);
		if (error != CL_SUCCESS) {
			std::cout << "Error in carry : write needCarry_d : " << error << std::endl;
			std::cin.get();
			exit(error);
		}

		error = clSetKernelArg(oclBigInt::carryKernel, 0, sizeof(cl_mem), &buffer);
		error |= clSetKernelArg(oclBigInt::carryKernel, 1, sizeof(cl_mem), &carry_d);
		error |= clSetKernelArg(oclBigInt::carryKernel, 2, sizeof(cl_mem), &oclBigInt::needCarry_d);
		error |= clSetKernelArg(oclBigInt::carryKernel, 3, sizeof(int), &minWidth);
		if (error != CL_SUCCESS) {
			std::cout << "Error in carry : set carry args : " << error << std::endl;
			std::cin.get();
			exit(error);
		}

		error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::carryKernel, 1, 0, &workItems,
			&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
		if (error != CL_SUCCESS) {
			std::cout << "Error in carry : enqueue carry : " << error << std::endl;
			std::cin.get();
			exit(error);
		}
		if (oclBigInt::profile == true) {
			clFlush(oclBigInt::queue);
			clFinish(oclBigInt::queue);
			clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
			clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
			oclBigInt::time_carry += (eT - sT);
		}
		clReleaseEvent(event_profile);
	//}

	//clEnqueueReadBuffer(oclBigInt::queue, buffer, CL_FALSE, 0, getNumLimbs() * sizeof(cl_uint), &c_h[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, carry_d, CL_TRUE, 0, getNumLimbs() * sizeof(cl_uint), &c2_h[0],
	//	0, NULL, NULL);
	//bool needCarry_h;
	//clEnqueueReadBuffer(oclBigInt::queue, oclBigInt::needCarry_d, CL_TRUE, 0, sizeof(bool), &needCarry_h,
	//	0, NULL, NULL);

	//if (needCarry_h == true) {
	//	//std::cout << "inspect." << std::endl;
	//	for (int i = 0; i < getNumLimbs(); i++) {
	//		std::vector<cl_uint> t(3);
	//		if (c2_h[i] != 0) {
	//			std::cout << "inspect." << std::endl;
	//		}
	//	}
	//}

	error = clSetKernelArg(oclBigInt::carry2Kernel, 0, sizeof(cl_mem), &buffer);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 1, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 2, sizeof(cl_mem), &oclBigInt::needCarry_d);
	error |= clSetKernelArg(oclBigInt::carry2Kernel, 3, sizeof(int), &minWidth);
	if (error != CL_SUCCESS) {
		std::cout << "Error in carry : set carry2 args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::carry2Kernel, 1, 0, &oclBigInt::workItemsPerGroup,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in carry : enqueue carry2 : " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_carry2 += (eT - sT);
		//timer += (eT - sT);
	}
	clReleaseEvent(event_profile);
}

void oclBigInt::init() {
	cl_int error;
	needCarry_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeof(bool), 0, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in init : create carry buffer : " << error << std::endl;
	}
}

void oclBigInt::close() {
	cl_int error;
	error = clReleaseMemObject(needCarry_d);
	if (error != CL_SUCCESS) {
		std::cout << "Error in close : release carry buffer : " << error << std::endl;
	}
}

void oclBigInt::resetProfiling() {
	time_baseMul = 0;
	time_carry = 0;
	time_carry2 = 0;
	time_fill = 0;
	time_shr = 0;
	time_shl = 0;
	time_count = 0;
	time_add = 0;
	time_neg = 0;
	time_carryOne = 0;
}

void oclBigInt::printProfiling(int runs, double bigTotal) {
	using std::cout; using std::endl;

	if (oclBigInt::profile == true) {
		cout.precision(3);
		double coefficient = 1.0 / 1000.0 / 1000.0 / 1000.0 / (double)runs;
		double total = (double)oclBigInt::time_baseMul + oclBigInt::time_carry2 + oclBigInt::time_fill
			+ oclBigInt::time_shl + oclBigInt::time_shr + oclBigInt::time_count + oclBigInt::time_neg 
			+ oclBigInt::time_add + time_carryOne;
		total = total * coefficient;
		double profileTime = total;
		double profile100 = total / bigTotal * 100.0;
		cout << "time_gpu : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_baseMul * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_mul    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_carry * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_carry : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_carry2 * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_carry2 : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_fill * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_fill   : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_shl * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_shl    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_shr * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_shr    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_count * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_count  : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_neg * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_neg    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_add * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_add    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)time_carryOne * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_carry1 : " << profileTime << "\t" << profile100 << "%" << endl;
	} else {
		cout << "profiling disabled" << endl;
	}
}

oclBigInt::oclBigInt(void) {
	numLimbs = 0;
}

oclBigInt::oclBigInt(size_t size) {
	numLimbs = 0;

	cl_int error;
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(unsigned int),
		NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(size_t): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(cl_uint),
		NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating carry in oclBigInt(size_t): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	setLimbs(newLimbs, newCarry, size);
}

oclBigInt::oclBigInt(int i, unsigned int pos) {
	numLimbs = 0;

	cl_int error;
	int size = pos + 1;
	std::vector<unsigned int> transfer(size, 0U);
	if (i < 0) {
		transfer[pos] = i * -1;
	} else {
		transfer[pos] = i;
	}
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		size * sizeof(unsigned int), &(transfer[0]), &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(unsigned int): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(cl_uint),
		NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating carry in oclBigInt(int, uint): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	setLimbs(newLimbs, newCarry, size);
	if (i < 0) {
		setNeg();
	}
}

oclBigInt::oclBigInt(double d, unsigned int prec) {
	numLimbs = 0;

	std::vector<unsigned int> transfer;
	for (int i = 0; i < prec; i++) {
		unsigned int t = (unsigned int)(floor(d));
		transfer.push_back(t);
		d -= t;
		d *= pow(2.0, 32);
	}
	int newSize = transfer.size();
	cl_int error;
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, newSize * sizeof(unsigned int), &(transfer[0]), &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(double): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, newSize * sizeof(cl_uint),
		NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating carry in oclBigInt(double): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	setLimbs(newLimbs, newCarry, newSize);
}

oclBigInt::oclBigInt(BigInt &n) {
	numLimbs = 0;
	int newSize = n.numLimbs();

	cl_int error;
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, newSize * sizeof(unsigned int), &(n.limbs[0]), &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating buffer in oclBigInt(BigInt): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, newSize * sizeof(cl_uint),
		NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error creating carry in oclBigInt(BigInt): " << error << std::endl;
		std::cin.get();
		exit(error);
	}
	setLimbs(newLimbs, newCarry, newSize);
}

oclBigInt::~oclBigInt(void)
{
	//std::cout << "release ocl" << std::endl;
	if (numLimbs > 0) {
		clReleaseMemObject(limbs);
		clReleaseMemObject(carry_d);
	}
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
	cl_event event_profile;
	cl_ulong sT, eT;

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

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shlKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in <<= : enqueue shl : " << error << endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_shl += (eT - sT);
	}
	clReleaseEvent(event_profile);
	clReleaseMemObject(src_d);

	return *this;
}

oclBigInt &oclBigInt::operator>>=(const int d) {
	using std::cout; using std::endl;
	cl_event event_profile;
	cl_ulong sT, eT;

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
		std::cout << "Error in >>= : set shr args : " << error << endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shrKernel, 1, 0, &workItems, 
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in >>= : enqueue shr : " << error << endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_shr += (eT - sT);
	}
	move(r);

	return *this;
}

oclBigInt &oclBigInt::operator+=(const oclBigInt &n) {
	int minWidth = n.numLimbs;
	if (n.numLimbs > numLimbs) {
		resize(n.numLimbs);
	}
	
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

	//oclBigInt t(n.getNumLimbs());
	//n.copy(t);
	//addCarry(limbs, t.limbs, t.getNumLimbs());

	fillBuffer(carry_d, 0, 0, minWidth);

	error = clSetKernelArg(oclBigInt::addKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::addKernel, 1, sizeof(cl_mem), &n.limbs);
	error |= clSetKernelArg(oclBigInt::addKernel, 2, sizeof(cl_mem), &carry_d);
	error |= clSetKernelArg(oclBigInt::addKernel, 3, sizeof(int), &minWidth);
	if (error != CL_SUCCESS) {
		std::cout << "Error in += : set add args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	size_t workItems = oclBigInt::getNumWorkItems(minWidth);
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::addKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in += : enqueue add : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_add += (eT - sT);
	}
	clReleaseEvent(event_profile);

	addCarry(limbs, carry_d, minWidth, oclBigInt::time_add);

	return *this;
}

oclBigInt &oclBigInt::operator-=(const oclBigInt &n) {
	oclBigInt t;
	n.copy(t);
	t.setNeg();
	//BigInt ct = t.toBigInt();
	//std::cout << ct << std::endl;
	return *this += t;
}

oclBigInt &oclBigInt::operator*=(const oclBigInt &n) {
	using std::cout; using std::endl;
	cl_int error;
	const int minSize = 0x30000;
	
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

	oclBigInt c_2, c_1, c_0;
	{
		oclBigInt a_1, a_0, b_1, b_0, t;
		copy(a_1);
		fillBuffer(a_1.limbs, 0x0, p, getNumLimbs());
		a_1 >>= 64;
		a_1.resize(p + 2);
		a_1.truncate(); // a_1 = m_1 / x^(p + 1)

		copy(a_0);
		fillBuffer(a_0.limbs, 0x0, 0, p);
		a_0 <<= (p * 32 - 64);
		a_0.resize(p+2);
		a_0.truncate(); // a_0 = m_0 / x^(p + 1)

		n.copy(b_1);
		fillBuffer(b_1.limbs, 0x0, p, n.getNumLimbs());
		b_1 >>= 64;
		b_1.resize(p+2);
		b_1.truncate(); // b_1 = n_1 / x^(p + 1)

		n.copy(b_0);
		fillBuffer(b_0.limbs, 0x0, 0, p);
		b_0 <<= (p * 32 - 64);
		b_0.resize(p+2);
		b_0.truncate(); // b_0 = n_0 / x^(p + 1)

		a_1.copy(c_2);
		c_2 *= b_1;     // c_2 = m_1 * n_1 / x^(2p + 2)

		a_0.copy(c_0);
		c_0 *= b_0;     // c_0 = m_0 * n_0 / x^(2p + 2)

		c_1.move(a_1);
		t.move(b_1);

		c_1 += a_0;
		t += b_0;
		c_1 *= t; // (m_1 + m_0)(n_1 + n_0) / x^(2p + 2)
	}

	c_1 -= c_2; // ((m_1 + m_0)(n_1 + n_0) - r_2) / x^(2p + 2)
	c_1 -= c_0; // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) / x^(2p + 2)

	c_2 <<= 4 * 32;           // c_2 = m_1 * n_1 * x^(2-2p)
	c_0 >>= (2 * p - 4) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
	c_1 >>= (p - 4) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

	c_0 += c_1;
	c_0 += c_2;

	move(c_0);

	return *this;
}

void oclBigInt::setNeg() {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

	size_t workItems = oclBigInt::getNumWorkItems(numLimbs);

	// invert
	error = clSetKernelArg(oclBigInt::negKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::negKernel, 1, sizeof(size_t), &numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : set invert args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	//BigInt y = this->toBigInt();

	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::negKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : enqueue neg : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_neg += (eT - sT);
	}
	clReleaseEvent(event_profile);

	//y = this->toBigInt();

	// carry
	error = clSetKernelArg(oclBigInt::carryOneKernel, 0, sizeof(cl_mem), &limbs);
	error |= clSetKernelArg(oclBigInt::carryOneKernel, 1, sizeof(size_t), &numLimbs);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : set carry args : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	error = clEnqueueTask(oclBigInt::queue, oclBigInt::carryOneKernel, 0, NULL, &event_profile);
	if (error != CL_SUCCESS) {
		std::cout << "Error in setNeg : enqueue carry : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_carryOne += (eT - sT);
	}
	clReleaseEvent(event_profile);
}

void oclBigInt::copy(oclBigInt &n) const {
	cl_int error; 

	if (n.getNumLimbs() != getNumLimbs()) {
		if (n.numLimbs > 0) {
			error = clReleaseMemObject(n.limbs);
			if (error != CL_SUCCESS) {
				std::cout << "Error in copy : release mem object : " << error << std::endl;
				std::cin.get();
				exit(error);
			}
			error = clReleaseMemObject(n.carry_d);
			if (error != CL_SUCCESS) {
				std::cout << "Error in copy : release mem object : " << error << std::endl;
				std::cin.get();
				exit(error);
			}
			n.numLimbs = 0;
		}

		int newSize = getNumLimbs();

		cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, newSize * sizeof(unsigned int), NULL, &error);
		if (error != CL_SUCCESS) {
			std::cout << "Error in copy : create buffer : " << error << std::endl;
			std::cin.get();
			exit(error);
		}
		cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, newSize * sizeof(cl_uint),
			NULL, &error);
		if (error != CL_SUCCESS) {
			std::cout << "Error in copy : create carry: " << error << std::endl;
			std::cin.get();
			exit(error);
		}
		n.setLimbs(newLimbs, newCarry, newSize);
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

oclBigInt &oclBigInt::shiftMul(const oclBigInt &n, int minSize) {
	using std::cout; using std::endl;
	//const int minSize = 0x2; // must be > 4
	cl_int error;

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (this->getNumLimbs() < minSize && n.getNumLimbs() < minSize) {
		oclBigInt t;
		n.copy(t);
		//BigInt ct = t.toBigInt();
		//ct >>= 64;
		t >>= 64;
		//if (!(ct == t.toBigInt())) {
			//cout << "stop." << endl;
		//}

		//BigInt cc_1 = this->toBigInt();
		//cc_1 >>= 64;
		*this >>= 64;
		//if (!(cc_1 == this->toBigInt())) {
			//cout << "stop." << endl;
		//}

		//cc_1 = this->toBigInt();
		//ct = t.toBigInt();
		//BigInt fin = cc_1.baseMul(ct);
		this->baseMul(t);
		//if (!(fin == this->toBigInt())) {
			//cc_1.out("c_1.txt");
			//ct.out("ct.txt");
			//cout << "stop." << endl;
		//}
		return *this;
	}

	int p;
	// temp way to match size
	if (n.getNumLimbs() > getNumLimbs()) { // num limbs / 2 rounded up
		p = n.getNumLimbs() / 2 + n.getNumLimbs() % 2;
	} else {
		p = getNumLimbs() / 2 + getNumLimbs() % 2;
	}

	oclBigInt a_1((size_t)p), a_0((size_t)p), b_1((size_t)p), b_0((size_t)p);

	// m_1 * x^(1 - p)
	clEnqueueCopyBuffer(oclBigInt::queue, this->limbs, a_1.limbs, 0, 0,
		p * sizeof(cl_uint), 0, NULL, NULL); 

	// m_0 * x^(1 - p);
	int sizeA_0 = getNumLimbs() - p;
	clEnqueueCopyBuffer(oclBigInt::queue, this->limbs, a_0.limbs, p * sizeof(cl_uint), 0,
		sizeA_0 * sizeof(cl_uint), 0, NULL, NULL);
	fillBuffer(a_0.limbs, 0, sizeA_0, p);    

	// n_1 * x^(1 - p)
	clEnqueueCopyBuffer(oclBigInt::queue, n.limbs, b_1.limbs, 0, 0,
		p * sizeof(cl_uint), 0, NULL, NULL); 

	// n_0 * x^(1 - p);
	int sizeB_0 = getNumLimbs() - p;
	clEnqueueCopyBuffer(oclBigInt::queue, n.limbs, b_0.limbs, p * sizeof(cl_uint), 0,
		sizeB_0 * sizeof(cl_uint), 0, NULL, NULL);
	fillBuffer(b_0.limbs, 0, sizeB_0, p);

	// setup c_1 and t
	//oclBigInt c_1((size_t)p), t((size_t)p);

#ifdef DEBUG_SHIFTMUL
	oclBigInt gc_2, gb_1;
	a_1.copy(gc_2);
	b_1.copy(gb_1);
	gc_2.shiftMul(gb_1, 0x1000000);
#endif
	oclBigInt c_2;
	a_1.copy(c_2);
	c_2.shiftMul(b_1, minSize); // c_2 = m_1 * n_1 * x^(-2 - 2p)
#ifdef DEBUG_SHIFTMUL
	if (!(gc_2.toBigInt() == c_2.toBigInt())) {
	//if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	oclBigInt gc_0, gb_0;
	a_0.copy(gc_0);
	b_0.copy(gb_0);
#endif
	oclBigInt c_0;
	a_0.copy(c_0);
#ifdef DEBUG_SHIFTMUL
	if (!(gc_0.toBigInt() == c_0.toBigInt()) || !(b_0.toBigInt() == gb_0.toBigInt())) {
		cout << "stop." << endl;
	}
	gc_0.shiftMul(gb_0, minSize);
#endif
	c_0.shiftMul(b_0, minSize); // c_0 = m_0 * n_0 * x^(-2 - 2p)
#ifdef DEBUG_SHIFTMUL
	if (!(gc_0.toBigInt() == c_0.toBigInt())) {
	//if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	BigInt ca_1, ca_0, cb_1, cb_0, cc_2, cc_0, cc_1, ct_a, ct_b, ct_c, co_a, co_b, ct;
#endif

	oclBigInt c_1; // c_1 = a_1
	c_1.move(a_1);

	oclBigInt t;  // t = b_1
	t.move(b_1);

	c_1 >>= 32; // m_1 * x^(-p)
	a_0 >>= 32; // m_0 * x^(-p)
	c_1 += a_0;
	oclBigInt o_a(1U); // overflow of addition
	clEnqueueCopyBuffer(oclBigInt::queue, c_1.limbs, o_a.limbs, 0, 0, sizeof(unsigned int),
		0, NULL, NULL);

	t >>= 32;   // n_1 * x^(-p)
	b_0 >>= 32; // n_0 * x^(-p)
	t += b_0;
	oclBigInt o_b(1U); // addition overflow
	clEnqueueCopyBuffer(oclBigInt::queue, t.limbs, o_b.limbs, 0, 0, sizeof(unsigned int),
		0, NULL, NULL);

	oclBigInt t_c;

	const unsigned int zero = 0;
	oclBigInt t_a(c_1.getNumLimbs());
	oclBigInt t_b(t.getNumLimbs());
	clEnqueueCopyBuffer(oclBigInt::queue, t.limbs, t_a.limbs, 1 * sizeof(unsigned int),
		1 * sizeof(unsigned int), (t.getNumLimbs() - 1) * sizeof(unsigned int), 0, NULL, NULL);
	clEnqueueWriteBuffer(oclBigInt::queue, t_a.limbs, CL_FALSE, 0, sizeof(unsigned int), &zero,
		0, NULL, NULL);
	clEnqueueCopyBuffer(oclBigInt::queue, c_1.limbs, t_b.limbs, 1 * sizeof(unsigned int),
		1 * sizeof(unsigned int), (c_1.getNumLimbs() - 1) * sizeof(unsigned int), 0, NULL, NULL);
	clEnqueueWriteBuffer(oclBigInt::queue, t_b.limbs, CL_FALSE, 0, sizeof(unsigned int), &zero,
		0, NULL, NULL);
	o_a.copy(t_c);

	//ct_a = t_a.toBigInt();
	//co_a = o_a.toBigInt();
	//ct_a = ct_a.baseMul(co_a);
	t_a.baseMul(o_a);
	//if (!(ct_a == t_a.toBigInt())) {
	//	cout << "stop." << endl;
	//}

	//ct_b = t_b.toBigInt();
	//co_b = o_b.toBigInt();
	//ct_b = ct_b.baseMul(co_b);
	t_b.baseMul(o_b);
	//if (!(ct_b == t_b.toBigInt())) {
	//	cout << "stop." << endl;
	//}

	//ct_c = t_c.toBigInt();
	//ct_c = ct_c.baseMul(co_b);
	t_c.baseMul(o_b);
	//if (!(ct_c == t_c.toBigInt())) {
	//	cout << "stop." << endl;
	//}

	t_a >>= 64;
	t_b >>= 64;
	t_c >>= 64;

	c_1 <<= 32; // c_1 = (m_1 + m_0) * x^(1-p)
	c_1.resize(p);
	t <<= 32;   // t = (n_1 + n_0) * x^(1-p)
	t.resize(p);

	//co_a = o_a.toBigInt();
	//co_b = o_b.toBigInt();
	//cc_1 = c_1.toBigInt();
	//ct_a = t_a.toBigInt();
	//ct_b = t_b.toBigInt();
	//ct_c = t_c.toBigInt();

	//cout << "o_a : " << co_a << endl
	//	<< "o_b : " << co_b << endl
	//	<< "c_1 : " << cc_1 << endl
	//	<< "t_a : " << ct_a << endl
	//	<< "t_b : " << ct_b << endl
	//	<< "t_c : " << ct_c << endl;

	//cc_1 = c_1.toBigInt();
	//ct = t.toBigInt();
	//cc_1 = cc_1.shiftMul(ct, minSize);
	//oclBigInt gc_1, gt;
	//c_1.copy(gc_1);
	//t.copy(gt);
	//gc_1.shiftMul(gt, 0x1000000);
	c_1.shiftMul(t, minSize); // (m_1 + m_0)(n_1 + n_0) * x^(-2 - 2p)
	//if (!(gc_1.toBigInt() == c_1.toBigInt())) {
	////if (!(cc_1 == c_1.toBigInt())) {
	//	cout << "stop." << endl;
	//}
	//cc_1 = c_1.toBigInt();
	//cout << "shiftMul : " << cc_1 << endl;

#ifdef DEBUG_SHIFTMUL
	cc_1 = c_1.toBigInt();
	ct_a = t_a.toBigInt();
	cc_1 += ct_a;
#endif
	c_1 += t_a;
#ifdef DEBUG_SHIFTMUL
	if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	//cc_1 = c_1.toBigInt();
	//cout << "+ t_a : " << cc_1 << endl;
#endif

#ifdef DEBUG_SHIFTMUL
	cc_1 = c_1.toBigInt();
	ct_b = t_b.toBigInt();
	cc_1 += ct_b;
#endif
	c_1 += t_b;
#ifdef DEBUG_SHIFTMUL
	if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	//cc_1 = c_1.toBigInt();
	//cout << "+ t_b : " << cc_1 << endl;
#endif

#ifdef DEBUG_SHIFTMUL
	cc_1 = c_1.toBigInt();
	ct_c = t_c.toBigInt();
	cc_1 += ct_c;
#endif
	c_1 += t_c;
#ifdef DEBUG_SHIFTMUL
	if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
#endif
	//cc_1 = c_1.toBigInt();
	//cout << "+ t_c : " << cc_1 << endl;

#ifdef DEBUG_SHIFTMUL
	cc_1 = c_1.toBigInt();
	cc_2 = c_2.toBigInt();
	cc_1 -= cc_2;
#endif
	c_1 -= c_2;      // ((m_1 + m_0)(n_1 + n_0) - r_2) * x^(-2 - 2p)
	//if (!(cc_1 == c_1.toBigInt())) {
	//	cout << "stop." << endl;
	//}
	//cc_1 = c_1.toBigInt();
	//cout << "- c_2 : " << cc_1 << endl;

#ifdef DEBUG_SHIFTMUL
	cc_1 = c_1.toBigInt();
	cc_0 = c_0.toBigInt();
	cc_1 -= cc_0;
#endif
	c_1 -= c_0;      // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(-2 - 2p)
#ifdef DEBUG_SHIFTMUL
	if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	cc_1 = c_1.toBigInt();
#endif
	                      // c_2 = m_1 * n_1 * x^(-2-2p)
	c_0 >>= (2 * p) * 32; // c_0 = m_0 * n_0 * x^(-2 - 4p)
	c_1 >>= p * 32;       // c_1 = ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) * x^(-2 - 3p)

	c_0 += c_2; // c_0 = R
	c_0 += c_1;

	move(c_0);

	return *this;
}

oclBigInt &oclBigInt::mul2(const oclBigInt &n, int minSize) {
	using std::cout; using std::endl;
	cl_int error;
	//const int minSize = 0x4000;

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (getNumLimbs() < minSize && n.getNumLimbs() < minSize) {
		this->baseMul(n);
		return *this;
	}

	int p; // max num limbs / 2 rounded up
	if (n.getNumLimbs() > getNumLimbs()) {
		p = n.getNumLimbs() / 2 + n.getNumLimbs() % 2;
	} else {
		p = getNumLimbs() / 2 + getNumLimbs() % 2;
	}

	this->shiftMul(n, minSize);
	*this <<= (4 * 32);
	this->resize(-1 + 4 * p);

	return *this;
}

void oclBigInt::move(oclBigInt &n) {
	setLimbs(n.limbs, n.carry_d, n.getNumLimbs());
	n.numLimbs = 0;
	n.setLimbs(NULL, NULL, 0);
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
cl_kernel oclBigInt::carryOneKernel;
cl_ulong oclBigInt::time_baseMul;
cl_ulong oclBigInt::time_carry;
cl_ulong oclBigInt::time_carry2;
cl_ulong oclBigInt::time_fill;
cl_ulong oclBigInt::time_shr;
cl_ulong oclBigInt::time_shl;
cl_ulong oclBigInt::time_count;
cl_ulong oclBigInt::time_add;
cl_ulong oclBigInt::time_neg;
cl_ulong oclBigInt::time_carryOne;
bool oclBigInt::profile = false;
cl_mem oclBigInt::needCarry_d;