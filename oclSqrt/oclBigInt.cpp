#include "StdAfx.h"
#include "oclBigInt.h"
#include "BigInt.h"

void oclBigInt::safeCL(const cl_int error, const char *call, const char *action) {
	if (error != CL_SUCCESS) {
		std::cout << "error : " << error << " in " << call << " : " << action << std::endl;
		std::cin.get();
		exit(error);
	}
}

void oclBigInt::initCheckCL(cl_kernel &kernel, const cl_int argOffset, const char *funcName,
	cl_mem &error_d, cl_mem &ixError_d, cl_uint &szError) {
	cl_int error;

	szError = 512;
	error_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, szError * sizeof(cl_int),
		NULL, &error);
	safeCL(error, funcName, "create error buffer");
	ixError_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeof(cl_int), NULL,
		&error);
	safeCL(error, funcName, "create ixError buffer");
	fillBuffer(error_d, 0, 0, szError);
	fillBuffer(ixError_d, 0, 0, 1);

	safeCL(clSetKernelArg(kernel, argOffset, sizeof(cl_mem), &error_d), funcName,
		"set 'error' arg");
	safeCL(clSetKernelArg(kernel, argOffset + 1, sizeof(cl_mem), &ixError_d), funcName,
		"set 'ixError' arg");
	safeCL(clSetKernelArg(kernel, argOffset + 2, sizeof(cl_uint), &szError), funcName,
		"set 'szError' arg");
}

void oclBigInt::checkCL(const char *funcName, const char *kernelName,
	cl_mem &error_d, cl_mem &ixError_d, cl_uint &szError) {
	std::vector<cl_int> error_h(szError);
	safeCL(clEnqueueReadBuffer(oclBigInt::queue, error_d, CL_TRUE, 0, szError * sizeof(cl_int), &error_h[0],
		0, NULL, NULL), funcName, "read error_d");
	for (int i = 0; i < szError && error_h[i] != CL_SUCCESS; i++) {
		safeCL(error_h[i], funcName, kernelName);
	}
	clReleaseMemObject(error_d);
	clReleaseMemObject(ixError_d);
}

void oclBigInt::fillBuffer(cl_mem buffer, unsigned int pattern, unsigned int offset, unsigned int size) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

	safeCL(clSetKernelArg(oclBigInt::fillKernel, 0, sizeof(cl_mem), &buffer), "fill", "set arg 'n'");
	safeCL(clSetKernelArg(oclBigInt::fillKernel, 1, sizeof(cl_uint), &pattern), "fill", "set arg 'pattern'");
	safeCL(clSetKernelArg(oclBigInt::fillKernel, 2, sizeof(cl_uint), &offset), "fill", "set arg 'offset'");
	safeCL(clSetKernelArg(oclBigInt::fillKernel, 3, sizeof(unsigned int), &size), "fill", "set arg 'szN'");

	size_t workItems = getNumWorkItems(size);
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::fillKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	safeCL(error, "fill", "enqueue kernel");

	if (oclBigInt::profile == true) {
		safeCL(clFlush(oclBigInt::queue), "fill profile", "flush");
		safeCL(clFinish(oclBigInt::queue), "fill profile", "finish");
		error = clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
			&sT, NULL);
		safeCL(error, "fill profile", "get sT");
		error = clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
			&eT, NULL);
		safeCL(error, "fill profile", "get eT");
		oclBigInt::time_fill += (eT - sT);
	}
	safeCL(clReleaseEvent(event_profile), "fill profile", "release event");
}

size_t oclBigInt::getNumWorkItems(size_t numLimbs) {
	size_t workItems = numLimbs;
	if (numLimbs % oclBigInt::workItemsPerGroup != 0) {
		workItems += oclBigInt::workItemsPerGroup - (numLimbs % oclBigInt::workItemsPerGroup);
	}
	return workItems;
}

//// old carry
//void oclBigInt::carry(cl_mem &limbs, cl_mem &carry_d, cl_mem &n2_d, int minWidth) {
//	cl_event event_profile;
//	cl_ulong sT, eT;
//	cl_int error;
//
//	size_t workItems = getNumWorkItems(minWidth);
//
//	int iterations;
//	for (iterations = 0; iterations < 2; iterations++) {
//		cl_mem swap = n2_d;
//		n2_d = carry_d;
//		carry_d = swap;
//		fillBuffer(carry_d, 0, 0, minWidth);
//
//		error = clSetKernelArg(oclBigInt::addKernel, 0, sizeof(cl_mem), &limbs);
//		error |= clSetKernelArg(oclBigInt::addKernel, 1, sizeof(cl_mem), &n2_d);
//		error |= clSetKernelArg(oclBigInt::addKernel, 2, sizeof(cl_mem), &carry_d);
//		error |= clSetKernelArg(oclBigInt::addKernel, 3, sizeof(int), &minWidth);
//		if (error != CL_SUCCESS) {
//			std::cout << "Error in carry : set add-carry args : " << error << std::endl;
//			std::cin.get();
//			exit(error);
//		}
//
//		error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::addKernel, 1, 0, &workItems,
//			&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
//		if (error != CL_SUCCESS) {
//			std::cout << "Error in += : enqueue add-carry : " << error << std::endl;
//			std::cin.get();
//			exit(error);
//		}
//
//		if (oclBigInt::profile == true) {
//			clFlush(oclBigInt::queue);
//			clFinish(oclBigInt::queue);
//			clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
//			clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
//			oclBigInt::time_add += (eT - sT);
//		}
//		clReleaseEvent(event_profile);
//	}
//
//	//clReleaseMemObject(n2_d);
//
//	addCarry(limbs, carry_d, minWidth, oclBigInt::time_baseMul);
//}

void oclBigInt::addCarry(cl_mem &buffer, cl_mem &carry_d, int minWidth, cl_ulong &timer) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;
	size_t workItems = getNumWorkItems(minWidth);

	shortAddOff(buffer, carry_d, minWidth, oclBigInt::needCarry_d, 1, timer);
}

void oclBigInt::shortAddOff(cl_mem &n, cl_mem &n2, int szN2, cl_mem &needAdd, int offset,
	cl_ulong &timer) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

	fillBuffer(needAdd, 0, 0, 1);
	safeCL(clSetKernelArg(oclBigInt::checkAddKernel, 0, sizeof(cl_mem), &n),
		"checkAdd", "set 'n' arg");
	safeCL(clSetKernelArg(oclBigInt::checkAddKernel, 1, sizeof(cl_mem), &n2),
		"checkAdd", "set 'carry_d' arg");
	safeCL(clSetKernelArg(oclBigInt::checkAddKernel, 2, sizeof(cl_mem), &needAdd),
		"checkAdd", "set 'needCarry' arg");
	safeCL(clSetKernelArg(oclBigInt::checkAddKernel, 3, sizeof(cl_int), &szN2),
		"checkAdd", "set 'szN' arg");
	safeCL(clSetKernelArg(oclBigInt::checkAddKernel, 4, sizeof(cl_int), &offset),
		"checkAdd", "set 'offset' arg");

	//std::vector<cl_uint> hn(szN2), hn2(szN2);
	//clEnqueueReadBuffer(oclBigInt::queue, n, CL_FALSE, 0, szN2 * sizeof(cl_uint), &hn[0], 0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, n2, CL_TRUE, 0, szN2 * sizeof(cl_uint), &hn2[0], 0, NULL, NULL);

	const cl_uint numWorkItems = getNumWorkItems(szN2);
	safeCL(clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::checkAddKernel, 1, 0,
		&numWorkItems, &oclBigInt::workItemsPerGroup, 0, NULL, &event_profile),
		"checkAdd", "enqueue 'checkAdd'");

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_carry += (eT - sT);
		//timer += (eT - sT);
	}
	clReleaseEvent(event_profile);

	//clEnqueueReadBuffer(oclBigInt::queue, n, CL_FALSE, 0, szN2 * sizeof(cl_uint), &hn[0], 0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, n2, CL_TRUE, 0, szN2 * sizeof(cl_uint), &hn2[0], 0, NULL, NULL);

	safeCL(clSetKernelArg(oclBigInt::shortAddOffKernel, 0, sizeof(cl_mem), &n),
		"shortAddOff", "set 'n' arg");
	safeCL(clSetKernelArg(oclBigInt::shortAddOffKernel, 1, sizeof(cl_mem), &n2),
		"shortAddOff", "set 'n2' arg");
	safeCL(clSetKernelArg(oclBigInt::shortAddOffKernel, 2, sizeof(cl_mem), &needAdd),
		"shortAddOff", "set 'needAdd' arg");
	safeCL(clSetKernelArg(oclBigInt::shortAddOffKernel, 3, sizeof(int), &szN2),
		"shortAddOff", "set 'szN2' arg");
	safeCL(clSetKernelArg(oclBigInt::shortAddOffKernel, 4, sizeof(int), &offset),
		"shortAddOff", "set 'offset' arg");

	safeCL(clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shortAddOffKernel, 1, 0,
		&oclBigInt::workItemsPerGroup, &oclBigInt::workItemsPerGroup, 0, NULL, &event_profile),
		"shortAddOff", "enqueue 'shortAddOff'");

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_carry2 += (eT - sT);
		//timer += (eT - sT);
	}
	clReleaseEvent(event_profile);

	//clEnqueueReadBuffer(oclBigInt::queue, n, CL_FALSE, 0, szN2 * sizeof(cl_uint), &hn[0], 0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, n2, CL_TRUE, 0, szN2 * sizeof(cl_uint), &hn2[0], 0, NULL, NULL);
}

void oclBigInt::setLimbs(cl_mem newLimbs, cl_mem newCarry, size_t newSize) {
	if (numLimbs > 0) {
		safeCL(clReleaseMemObject(limbs), "setLimbs", "release limbs");
		safeCL(clReleaseMemObject(carry_d), "setLimbs", "release carry");
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
	safeCL(error, "resize", "create newLimbs buffer");
	if (numLimbs > 0) {
		int copySize;
		if (numLimbs < size) {
			copySize = numLimbs * sizeof(unsigned int);
			fillBuffer(newLimbs, 0, numLimbs, size);
		} else {
			copySize = size * sizeof(unsigned int);
		}
		error = clEnqueueCopyBuffer(oclBigInt::queue, limbs, newLimbs, 0, 0, copySize, 0, NULL, NULL);
		safeCL(error, "resize", "enqueue copy limbs>newLimbs");
	} else {
		fillBuffer(newLimbs, 0, 0, size);
	}
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(cl_uint),
		NULL, &error);
	safeCL(error, "resize", "create copy buffer");
	setLimbs(newLimbs, newCarry, size);
}

void oclBigInt::rmask(const oclBigInt &mask) {
	cl_int error;

	cl_mem newSize_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, sizeof(unsigned int), NULL,
		&error);
	safeCL(error, "rmask", "create newSize buffer");

	safeCL(clSetKernelArg(oclBigInt::rmaskKernel, 0, sizeof(cl_mem), &limbs), "rmask", "set 'n' arg");
	safeCL(clSetKernelArg(oclBigInt::rmaskKernel, 1, sizeof(cl_mem), &mask.limbs),
		"rmask", "set 'mask' arg");
	safeCL(clSetKernelArg(oclBigInt::rmaskKernel, 2, sizeof(int), &numLimbs), "rmask", "set 'sizeN' arg");
	safeCL(clSetKernelArg(oclBigInt::rmaskKernel, 3, sizeof(cl_mem), &newSize_d),
		"rmask", "set 'newSize' arg");

	size_t workItems = oclBigInt::getNumWorkItems(numLimbs);
	safeCL(clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::rmaskKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, NULL), "rmask", "enqueue 'rmask' kernel");

	int newSize = 0;
	safeCL(clEnqueueReadBuffer(oclBigInt::queue, newSize_d, CL_TRUE, 0, sizeof(unsigned int), &newSize,
		0, NULL, NULL), "rmask", "read 'newSize' buffer");

	safeCL(clReleaseMemObject(newSize_d), "rmask", "release 'newSize' buffer");

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

	cl_mem carry2_d = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, r.getNumLimbs() * sizeof(unsigned int), NULL, &error);
	if (error != CL_SUCCESS) {
		std::cout << "Error in * : create carry2 buffer : " << error << std::endl;
		std::cin.get();
		exit(error);
	}

	fillBuffer(r.carry_d, 0, 0, r.getNumLimbs());
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

	//std::vector<cl_uint> hn(r.numLimbs), hcarry(r.numLimbs), hcarry2(r.numLimbs);
	//clEnqueueReadBuffer(oclBigInt::queue, r.limbs, CL_FALSE, 0, r.numLimbs * sizeof(cl_uint), &hn[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, r.carry_d, CL_FALSE, 0, r.numLimbs * sizeof(cl_uint), &hcarry[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, carry2_d, CL_TRUE, 0, r.numLimbs * sizeof(cl_uint), &hcarry2[0],
	//	0, NULL, NULL);

	//fillBuffer(oclBigInt::needCarry_d, 1, 0, 1); // set needCarry_d to true
	
	addCarry(r.limbs, r.carry_d, r.numLimbs, oclBigInt::time_baseMul);

	//clEnqueueReadBuffer(oclBigInt::queue, r.limbs, CL_FALSE, 0, r.numLimbs * sizeof(cl_uint), &hn[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, r.carry_d, CL_FALSE, 0, r.numLimbs * sizeof(cl_uint), &hcarry[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, carry2_d, CL_TRUE, 0, r.numLimbs * sizeof(cl_uint), &hcarry2[0],
	//	0, NULL, NULL);

	
	//shortAddOff(r.carry_d, carry2_d, r.numLimbs, oclBigInt::needCarry_d, 0, oclBigInt::time_baseMul);
	//carry(r.limbs, r.carry_d, carry2_d, r.getNumLimbs());
	addCarry(r.limbs, carry2_d, r.numLimbs, oclBigInt::time_baseMul);

	//clEnqueueReadBuffer(oclBigInt::queue, r.limbs, CL_FALSE, 0, r.numLimbs * sizeof(cl_uint), &hn[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, r.carry_d, CL_FALSE, 0, r.numLimbs * sizeof(cl_uint), &hcarry[0],
	//	0, NULL, NULL);
	//clEnqueueReadBuffer(oclBigInt::queue, carry2_d, CL_TRUE, 0, r.numLimbs * sizeof(cl_uint), &hcarry2[0],
	//	0, NULL, NULL);

	move(r);

	clReleaseMemObject(carry2_d);

	return *this;

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
		cout << "\ttime_mul      : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_carry * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_carry    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_carry2 * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_carry2   : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)time_carryOne * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_carryOne : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_fill * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_fill     : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_shl * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_shl      : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_shr * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_shr      : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_count * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_count    : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_neg * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_neg      : " << profileTime << "\t" << profile100 << "%" << endl;

		profileTime = (double)oclBigInt::time_add * coefficient;
		profile100 = profileTime / total * 100.0;
		cout << "\ttime_add      : " << profileTime << "\t" << profile100 << "%" << endl;
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
	cl_int error;

	numLimbs = 0;

	int size = pos + 1;
	std::vector<unsigned int> transfer(size, 0U);
	if (i < 0) {
		transfer[pos] = i * -1;
	} else {
		transfer[pos] = i;
	}
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		size * sizeof(unsigned int), &(transfer[0]), &error);
	safeCL(error, "oclBigInt(int, uint)", "create newLimbs buffer");

	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, size * sizeof(cl_uint),
		NULL, &error);
	safeCL(error, "oclBigInt(int, uint)", "create newCarry buffer");

	setLimbs(newLimbs, newCarry, size);
	if (i < 0) {
		setNeg();
	}
}

oclBigInt::oclBigInt(double d, unsigned int prec) {
	cl_int error;

	numLimbs = 0;

	std::vector<unsigned int> transfer;
	for (int i = 0; i < prec; i++) {
		unsigned int t = (unsigned int)(floor(d));
		transfer.push_back(t);
		d -= t;
		d *= pow(2.0, 32);
	}
	int newSize = transfer.size();

	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, newSize * sizeof(unsigned int), &(transfer[0]), &error);
	safeCL(error, "oclBigInt(double, uint)", "create newLimbs buffer");

	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, newSize * sizeof(cl_uint),
		NULL, &error);
	safeCL(error, "oclBigInt(double, uint)", "create newCarry buffer");

	setLimbs(newLimbs, newCarry, newSize);
}

oclBigInt::oclBigInt(BigInt &n) {
	numLimbs = 0;

	int newSize = n.numLimbs();

	cl_int error;
	cl_mem newLimbs = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		newSize * sizeof(unsigned int), &n.limbs[0], &error);
	safeCL(error, "oclBigInt(BigInt)", "create limbs buffer");
	//safeCL(clEnqueueWriteBuffer(oclBigInt::queue, newLimbs, CL_FALSE, 0, newSize * sizeof(cl_uint),
	//	&n.limbs[0], 0, NULL, NULL), "oclBigInt(BigInt)", "write newLimbs buffer");
	cl_mem newCarry = clCreateBuffer(oclBigInt::context, CL_MEM_READ_WRITE, newSize * sizeof(cl_uint),
		NULL, &error);
	safeCL(error, "oclBigInt(BigInt)", "create carry buffer");
	setLimbs(newLimbs, newCarry, newSize);
}

oclBigInt::~oclBigInt(void)
{
	//std::cout << "release ocl" << std::endl;
	if (numLimbs > 0) {
		safeCL(clReleaseMemObject(limbs), "~oclBigInt", "release limbs buffer");
		safeCL(clReleaseMemObject(carry_d), "~oclBigInt", "release carry buffer");
	}
}

bool oclBigInt::operator>(const int n) {
	cl_int error;

	cl_uint limb0;
	error = clEnqueueReadBuffer(oclBigInt::queue, limbs, CL_TRUE, 0, sizeof(unsigned int), &limb0,
		0, NULL, NULL);
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
	cl_int error;

	if (d < 0) {
		return *this >>= (d * -1);
	} else if (d == 0) {
		return *this;
	}

	//oclBigInt r((size_t)numLimbs - d / 32);
	cl_uint szDst = numLimbs - d / 32;
	fillBuffer(carry_d, 0, 0, numLimbs);

	safeCL(clSetKernelArg(oclBigInt::shlKernel, 0, sizeof(cl_mem), &limbs), "<<=", "set 'src' arg");
	safeCL(clSetKernelArg(oclBigInt::shlKernel, 1, sizeof(cl_mem), &carry_d), "<<=", "set 'dst' arg");
	safeCL(clSetKernelArg(oclBigInt::shlKernel, 2, sizeof(cl_uint), &szDst),
		"<<=", "set 'szDst' arg");
	safeCL(clSetKernelArg(oclBigInt::shlKernel, 3, sizeof(cl_int), &d), "<<=", "set 'd' arg");
#ifdef DEBUG_CL
	cl_mem error_d, ixError_d;
	cl_uint szError;
	initCheckCL(oclBigInt::shlKernel, 4, "<<=", error_d, ixError_d, szError);
#endif

	size_t workItems = getNumWorkItems(numLimbs);
	safeCL(clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shlKernel, 1, 0, &workItems,
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile), "<<=", "enqueue shl kernel");

	if (oclBigInt::profile == true) {
		safeCL(clFlush(oclBigInt::queue), "<<= profile", "flush");
		safeCL(clFinish(oclBigInt::queue), "<<= profile", "finish");
		safeCL(clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
			&sT, NULL), "<<= profile", "get sT");
		safeCL(clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
			&eT, NULL), "<<= profile", "get eT");
		oclBigInt::time_shl += (eT - sT);
	}
	safeCL(clReleaseEvent(event_profile), "<<= profile", "release event");

	cl_mem swap = limbs;
	limbs = carry_d;
	carry_d = swap;

#ifdef DEBUG_CL
	checkCL("<<=", "shl kernel", error_d, ixError_d, szError);
#endif
	
	return *this;
}

oclBigInt &oclBigInt::operator>>=(const int d) {
	using std::cout; using std::endl;
	cl_event event_profile;
	cl_ulong sT, eT;
	cl_int error;

	if (d < 0) {
		return *this <<= (d * -1);
	} else if (d == 0) {
		return *this;
	}

	oclBigInt r;
	r.resize(numLimbs + d / 32 + (d % 32 > 0 ? 1 : 0));

	safeCL(clSetKernelArg(oclBigInt::shrKernel, 0, sizeof(cl_mem), &limbs), ">>=", "set 'src' arg");
	safeCL(clSetKernelArg(oclBigInt::shrKernel, 1, sizeof(cl_mem), &r.limbs), ">>=", "set 'dst' arg");
	safeCL(clSetKernelArg(oclBigInt::shrKernel, 2, sizeof(cl_uint), &r.numLimbs), ">>=", "set 'szDst' arg");
	safeCL(clSetKernelArg(oclBigInt::shrKernel, 3, sizeof(cl_int), &d), ">>=", "set 'd' arg");

#ifdef DEBUG_CL
	cl_mem error_d, ixError_d;
	cl_uint szError;
	initCheckCL(oclBigInt::shrKernel, 4, ">>=", error_d, ixError_d, szError);
#endif

	size_t workItems = getNumWorkItems(r.numLimbs);
	error = clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::shrKernel, 1, 0, &workItems, 
		&oclBigInt::workItemsPerGroup, 0, NULL, &event_profile);
	safeCL(error, ">>=", "enqueue shr");

	if (oclBigInt::profile == true) {
		safeCL(clFlush(oclBigInt::queue), ">>= profile", "flush");
		safeCL(clFinish(oclBigInt::queue), ">>= profile", "finish");
		safeCL(clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong),
			&sT, NULL), ">>= profile", "get sT");
		safeCL(clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong),
			&eT, NULL), ">>= profile", "get eT");
		oclBigInt::time_shr += (eT - sT);
	}
	move(r);

#ifdef DEBUG_CL
	checkCL(">>=", "shr kernel", error_d, ixError_d, szError);
#endif

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
	const int minSize = 0xc000;

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	if (getNumLimbs() < minSize && n.getNumLimbs() < minSize) {
		this->baseMul(n);
		this->truncate();
		return *this;
	}

	int p; // max num limbs / 2 rounded up
	if (n.getNumLimbs() > getNumLimbs()) {
		p = n.getNumLimbs() / 2 + n.getNumLimbs() % 2;
	} else {
		p = getNumLimbs() / 2 + getNumLimbs() % 2;
	}

	oclBigInt tN;
	n.copy(tN);
	this->shiftMul(tN, minSize);
	*this <<= (4 * 32);
	this->truncate();
	//this->resize(-1 + 4 * p);

	return *this;

	//using std::cout; using std::endl;
	//cl_int error;
	//const int minSize = 0x30000;
	//
	//if (getNumLimbs() < minSize && n.getNumLimbs() < minSize) {
	//	this->baseMul(n);
	//	return *this;
	//}

	//// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	//// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	//int p; // max num limbs / 2 rounded up
	//if (n.getNumLimbs() > getNumLimbs()) {
	//	p = n.getNumLimbs() / 2 + n.getNumLimbs() % 2;
	//} else {
	//	p = getNumLimbs() / 2 + getNumLimbs() % 2;
	//}

	//oclBigInt c_2, c_1, c_0;
	//{
	//	oclBigInt a_1, a_0, b_1, b_0, t;
	//	copy(a_1);
	//	fillBuffer(a_1.limbs, 0x0, p, getNumLimbs());
	//	a_1 >>= 64;
	//	a_1.resize(p + 2);
	//	a_1.truncate(); // a_1 = m_1 / x^(p + 1)

	//	copy(a_0);
	//	fillBuffer(a_0.limbs, 0x0, 0, p);
	//	a_0 <<= (p * 32 - 64);
	//	a_0.resize(p+2);
	//	a_0.truncate(); // a_0 = m_0 / x^(p + 1)

	//	n.copy(b_1);
	//	fillBuffer(b_1.limbs, 0x0, p, n.getNumLimbs());
	//	b_1 >>= 64;
	//	b_1.resize(p+2);
	//	b_1.truncate(); // b_1 = n_1 / x^(p + 1)

	//	n.copy(b_0);
	//	fillBuffer(b_0.limbs, 0x0, 0, p);
	//	b_0 <<= (p * 32 - 64);
	//	b_0.resize(p+2);
	//	b_0.truncate(); // b_0 = n_0 / x^(p + 1)

	//	a_1.copy(c_2);
	//	c_2 *= b_1;     // c_2 = m_1 * n_1 / x^(2p + 2)

	//	a_0.copy(c_0);
	//	c_0 *= b_0;     // c_0 = m_0 * n_0 / x^(2p + 2)

	//	c_1.move(a_1);
	//	t.move(b_1);

	//	c_1 += a_0;
	//	t += b_0;
	//	c_1 *= t; // (m_1 + m_0)(n_1 + n_0) / x^(2p + 2)
	//}

	//c_1 -= c_2; // ((m_1 + m_0)(n_1 + n_0) - r_2) / x^(2p + 2)
	//c_1 -= c_0; // ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0) / x^(2p + 2)

	//c_2 <<= 4 * 32;           // c_2 = m_1 * n_1 * x^(2-2p)
	//c_0 >>= (2 * p - 4) * 32; // c_0 = m_0 * n_0 * x^(2 - 4p)
	//c_1 >>= (p - 4) * 32;     // c_1 = x^(2 - 3p) * ((m_1 + m_0)(n_1 + n_0) - r_2 - r_0)

	//c_0 += c_1;
	//c_0 += c_2;

	//move(c_0);

	//return *this;
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

#ifdef DEBUG_VERIFY
	BigInt y = e.toBigInt();
	y = y * y;
#endif

	e *= *this;

#ifdef DEBUG_VERIFY
	if (y == e.toBigInt()) ;
	y = e.toBigInt();
	y <<= 1;
#endif

	e <<= 1;

#ifdef DEBUG_VERIFY
	if (y == e.toBigInt()) ;
	y = e.toBigInt();
	y.setNeg();
#endif

	e.setNeg();

#ifdef DEBUG_VERIFY
	if (y == e.toBigInt()) ;
	y = e.toBigInt();
	y += BigInt(1U);
#endif

	e += oclBigInt(1);
	
#ifdef DEBUG_VERIFY
	BigInt ce = e.toBigInt();
	if (y == ce) ;
#endif

	if (e > 1) {
#ifdef DEBUG_VERIFY
		BigInt y = e.toBigInt();
		y.setNeg();
#endif

		e.setNeg();
		
#ifdef DEBUG_VERIFY
		y == e.toBigInt();
#endif
	}

	rmask(e);
	//truncate();
}

void oclBigInt::split(const oclBigInt &n, oclBigInt &n_1, oclBigInt &n_0, int p) {
	n_1.resize(p);
	n_0.resize(p);

	//if (n.getNumLimbs() > p) {
		// n_1 * x^(1 - p)
		// get p uints from n.limbs and copy them to n_1
		safeCL(clEnqueueCopyBuffer(oclBigInt::queue, n.limbs, n_1.limbs, 0, 0,
			p * sizeof(cl_uint), 0, NULL, NULL), "split", "copy n_1"); 

		// n_0 * x^(1 - p);
		// grab remaining uints from n.limbs (offset to p) and copy them to n_0
		int sizeN_0 = n.getNumLimbs() - p;
		safeCL(clEnqueueCopyBuffer(oclBigInt::queue, n.limbs, n_0.limbs, p * sizeof(cl_uint), 0,
			sizeN_0 * sizeof(cl_uint), 0, NULL, NULL), "split", "copy n_0");
		// fill the remaining space with 0
		fillBuffer(n_0.limbs, 0, sizeN_0, p);
	//} else {
	//	// copy over all n.limbs
	//	safeCL(clEnqueueCopyBuffer(oclBigInt::queue, n.limbs, n_1.limbs, 0, 0,
	//		n.getNumLimbs() * sizeof(cl_uint), 0, NULL, NULL), "split", "copy n_1");
	//	fillBuffer(n_1.limbs, 0, n.getNumLimbs(), p);

	//	fillBuffer(n_0.limbs, 0, 0, p);

	//	BigInt cn = n.toBigInt();
	//	BigInt cn_1 = n_1.toBigInt();
	//	BigInt cn_0 = n_0.toBigInt();
	//}
}

void oclBigInt::toggle(cl_mem dCarry) {
	cl_int error;
	cl_event event_profile;
	cl_ulong sT, eT;

	safeCL(clSetKernelArg(oclBigInt::toggleKernel, 0, sizeof(cl_mem), &limbs),
		"toggle", "set 'n' arg");
	safeCL(clSetKernelArg(oclBigInt::toggleKernel, 1, sizeof(cl_int), &numLimbs),
		"toggle", "set 'szN' arg");
	safeCL(clSetKernelArg(oclBigInt::toggleKernel, 2, sizeof(cl_mem), &dCarry),
		"toggle", "set 'carry' arg");

	const cl_uint numWorkItems = getNumWorkItems(numLimbs);
	safeCL(clEnqueueNDRangeKernel(oclBigInt::queue, oclBigInt::toggleKernel, 1, 0,
		&numWorkItems, &oclBigInt::workItemsPerGroup, 0, NULL, &event_profile),
		"toggle", "enqueue 'toggle'");

	if (oclBigInt::profile == true) {
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &sT, NULL);
		clGetEventProfilingInfo(event_profile, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &eT, NULL);
		oclBigInt::time_baseMul += (eT - sT);
		//timer += (eT - sT);
	}
	clReleaseEvent(event_profile);
}

oclBigInt &oclBigInt::shiftMul(oclBigInt &n, int minSize) {
	using std::cout; using std::endl;
	cl_int error;

	// *this = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// n = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)

	// base case
	if (this->getNumLimbs() < minSize && n.getNumLimbs() < minSize) {
#ifdef DEBUG_SHIFTMUL
		BigInt ct = t.toBigInt();
		ct >>= 64;
#endif
		n >>= 64;
#ifdef DEBUG_SHIFTMUL
		if (!(ct == t.toBigInt())) {
			cout << "stop." << endl;
		}
		BigInt cc_1 = this->toBigInt();
		cc_1 >>= 64;
#endif
		*this >>= 64;
#ifdef DEBUG_SHIFTMUL
		if (!(cc_1 == this->toBigInt())) {
			cout << "stop." << endl;
		}

		cc_1 = this->toBigInt();
		ct = t.toBigInt();
		BigInt fin = cc_1.baseMul(ct);
#endif
		this->baseMul(n);
#ifdef DEBUG_SHIFTMUL
		if (!(fin == this->toBigInt())) {
			cc_1.out("c_1.txt");
			ct.out("ct.txt");
			cout << "stop." << endl;
		}
#endif
		return *this;
	}

	const int maxSize = n.getNumLimbs() > getNumLimbs() ? n.getNumLimbs() : getNumLimbs();
	int p = (maxSize + 1) / 2; // max size / 2 rounded up

	if (this->getNumLimbs() <= p || n.getNumLimbs() <= p) {
		// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
		// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)
		// r_2 = m_1 * n_1
		// r_1 = m_0 * n_1
		// c_2 = r_2 * x^(-2 - 2p)
		// c_1 = r_1 * x^(-2 - 3p)
		// R = c_2 + c_1

		// a_1 = m_1 * x^(1 - p)
		// a_0 = m_0 * x^(2 - p)
		// b_1 = n_1 * x^(1 - p)
		oclBigInt a_1, a_0, b_1;

		if (this->getNumLimbs() <= p) {
			split(n, a_1, a_0, p);
			this->copy(b_1);
		} else {
			split(*this, a_1, a_0, p);
			n.copy(b_1);
		}

		oclBigInt c_2, c_1, t;

		// c_2 = m_1 * n_1 * x^(-2 - 2p)
		b_1.copy(t);
		c_2.move(a_1);
		c_2.shiftMul(t, minSize);

		// c_1 = m_0 * n_1 * x^(-2 - 3p)
		c_1.move(a_0);
		c_1.shiftMul(b_1, minSize);
		c_1 >>= p * 32;

		// r = c_2 + c_1
		move(c_1);
		*this += c_2;

		return *this;
	}

	// M = m_1 * x^(1 - p) + m_0 * x^(1 - 2p)
	// N = n_1 * x^(1 - p) + n_0 * x^(1 - 2p)
	// r_2 = m_1 * n_1
	// r_0 = m_0 * n_0
	// r_1 = (m_1 + m_0)(n_1 + n_0) - r_2 - r_0
	// c_2 = r_2 * x^(-2 - 2p)
	// c_0 = r_0 * x^(-2 - 4p)
	// c_1 = r_1 * x^(-2 - 3p)
	// R = c_2 + c_1 + c_0

	// a_1 = m_1 * x^(1 - p)
	// a_0 = m_0 * x^(1 - 2p)
	// b_1 = n_1 * x^(1 - p)
	// b_0 = n_0 * x^(1 - 2p)
	oclBigInt a_1, a_0, b_1, b_0;
	split(*this, a_1, a_0, p);
	split(n, b_1, b_0, p);

#ifdef DEBUG_SHIFTMUL
	oclBigInt gc_2, gb_1;
	a_1.copy(gc_2);
	b_1.copy(gb_1);
	gc_2.shiftMul(gb_1, 0x1000000);
#endif
	//oclBigInt c_2, tb_1, c_0, tb_0, c_1, t;

	oclBigInt c_2, tb_1;
	a_1.copy(c_2);
	b_1.copy(tb_1);
	c_2.shiftMul(tb_1, minSize); // c_2 = m_1 * n_1 * x^(-2 - 2p)
#ifdef DEBUG_SHIFTMUL
	if (!(gc_2.toBigInt() == c_2.toBigInt())) {
	//if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	oclBigInt gc_0, gb_0;
	a_0.copy(gc_0);
	b_0.copy(gb_0);
#endif
	oclBigInt c_0, tb_0;
	a_0.copy(c_0);
	b_0.copy(tb_0);
#ifdef DEBUG_SHIFTMUL
	if (!(gc_0.toBigInt() == c_0.toBigInt()) || !(b_0.toBigInt() == gb_0.toBigInt())) {
		cout << "stop." << endl;
	}
	gc_0.shiftMul(gb_0, minSize);
#endif
	c_0.shiftMul(tb_0, minSize); // c_0 = m_0 * n_0 * x^(-2 - 2p)
#ifdef DEBUG_SHIFTMUL
	if (!(gc_0.toBigInt() == c_0.toBigInt())) {
	//if (!(cc_1 == c_1.toBigInt())) {
		cout << "stop." << endl;
	}
	BigInt ca_1, ca_0, cb_1, cb_0, cc_2, cc_0, cc_1, ct_a, ct_b, ct_c, co_a, co_b, ct;
#endif

	//oclBigInt t_a, t_b, t_c(1U);
	//c_1.move(a_1);
	//t.move(b_1);

	//c_1 += a_0;
	//c_1.copy(t_b);
	//t_b.toggle(c_1.carry_d);

	//t += b_0;
	//t.copy(t_a);
	//t_a.toggle(t.carry_d);

	//safeCL(clEnqueueCopyBuffer(oclBigInt::queue, c_1.carry_d, t_c.limbs, 0, 0, 1 * sizeof(cl_uint),
	//	0, NULL, NULL), "shiftMul", "copy t_c");
	//t_c.toggle(t.carry_d);

	//t_a >>= 64;
	//t_b >>= 64;
	//t_c >>= 64;

	// *******
	// * old *
	// *******
	oclBigInt c_1, t, t_a((size_t)b_1.getNumLimbs() + 3), t_b((size_t)a_1.getNumLimbs() + 3), t_c(3U); // c_1 = a_1
	c_1.move(a_1);
	//c_1 >>= 32; // m_1 * x^(-p)
	//a_0 >>= 32; // m_0 * x^(-p)
	c_1 += a_0;
	fillBuffer(t_b.limbs, 0, 0, t_b.getNumLimbs() - c_1.getNumLimbs());
	safeCL(clEnqueueCopyBuffer(oclBigInt::queue, c_1.limbs, t_b.limbs, 0, 3 * sizeof(cl_uint),
		c_1.numLimbs * sizeof(cl_uint), 0, NULL, NULL), "shiftMul", "copy to t_b");
	//c_1.copy(t_b);
	//c_1 <<= 32; // c_1 = (m_1 + m_0) * x^(1-p)
	//c_1.resize(p);

	//oclBigInt t, t_a;  // t = b_1

	t.move(b_1);
	//t >>= 32;   // n_1 * x^(-p)
	//b_0 >>= 32; // n_0 * x^(-p)
	t += b_0;
	fillBuffer(t_a.limbs, 0, 0, t_a.getNumLimbs() - t.getNumLimbs());
	safeCL(clEnqueueCopyBuffer(oclBigInt::queue, t.limbs, t_a.limbs, 0, 3 * sizeof(cl_uint),
		t.numLimbs * sizeof(cl_uint), 0, NULL, NULL), "shiftMul", "copy to t_a");
	//t.copy(t_a);
	//oclBigInt o_b(1U); // addition overflow
	//clEnqueueCopyBuffer(oclBigInt::queue, t.carry_d, o_b.limbs, 0, 0, sizeof(unsigned int),
	//	0, NULL, NULL);
	//t <<= 32;   // t = (n_1 + n_0) * x^(1-p)
	//t.resize(p);

	//oclBigInt t_c(1U);

	fillBuffer(t_c.limbs, 0, 0, 2);
	safeCL(clEnqueueCopyBuffer(oclBigInt::queue, c_1.carry_d, t_c.limbs, 0, 2 * sizeof(cl_uint), sizeof(unsigned int),
		0, NULL, NULL), "shiftMul", "copy overflow to t_c");

	//const unsigned int zero = 0;
	//oclBigInt t_a(t.getNumLimbs() + 1);
	//oclBigInt t_b(c_1.getNumLimbs() + 1);
	//clEnqueueCopyBuffer(oclBigInt::queue, t.limbs, t_a.limbs, 0 * sizeof(unsigned int),
	//	1 * sizeof(unsigned int), (t.getNumLimbs()) * sizeof(unsigned int), 0, NULL, NULL);
	//clEnqueueWriteBuffer(oclBigInt::queue, t_a.limbs, CL_FALSE, 0, sizeof(unsigned int), &zero,
	//	0, NULL, NULL);
	//clEnqueueCopyBuffer(oclBigInt::queue, c_1.limbs, t_b.limbs, 0 * sizeof(unsigned int),
	//	1 * sizeof(unsigned int), (c_1.getNumLimbs()) * sizeof(unsigned int), 0, NULL, NULL);
	//clEnqueueWriteBuffer(oclBigInt::queue, t_b.limbs, CL_FALSE, 0, sizeof(unsigned int), &zero,
	//	0, NULL, NULL);
	//o_a.copy(t_c);

	t_a.toggle(c_1.carry_d);
	//t_a.baseMul(o_a);
	t_b.toggle(t.carry_d);
	//t_b.baseMul(o_b);
	t_c.toggle(t.carry_d);
	//t_c.baseMul(o_b);

	//t_a >>= (64 + 32);
	//t_b >>= (64 + 32);
	//t_c >>= 64;

	//BigInt cc_1 = c_1.toBigInt(), ct = t.toBigInt(), ct_a = t_a.toBigInt(), ct_b = t_b.toBigInt(),
	//	ct_c = t_c.toBigInt();

	c_1.shiftMul(t, minSize); // (m_1 + m_0)(n_1 + n_0) * x^(-2 - 2p)

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
	                      // c_2 = m_1 * n_1 * x^(-2 - 2p)
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

	oclBigInt tN;
	n.copy(tN);
	this->shiftMul(tN, minSize);
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
//cl_kernel oclBigInt::mulKernel;
cl_kernel oclBigInt::rmaskKernel;
cl_kernel oclBigInt::countKernel;
cl_kernel oclBigInt::mul2Kernel;
//cl_kernel oclBigInt::carry2Kernel;
//cl_kernel oclBigInt::oldMulKernel;
cl_kernel oclBigInt::carryOneKernel;
cl_kernel oclBigInt::shortAddOffKernel;
cl_kernel oclBigInt::checkAddKernel;
cl_kernel oclBigInt::toggleKernel;
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
bool oclBigInt::profile = true;
cl_mem oclBigInt::needCarry_d;