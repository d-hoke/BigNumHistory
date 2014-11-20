// oclSqrt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BigInt.h"
#include "oclBigInt.h"

cl_int getPlatform(cl_platform_id &_platformId);
cl_int getDevice(cl_device_id &_deviceId, const cl_platform_id &_platformId);
cl_int getContext(cl_context &_context, cl_device_id *_devices, cl_uint _numDevices);
cl_int getQueue(cl_command_queue &_queue, const cl_context &_context, const cl_device_id &_deviceId);
cl_int oclInit(cl_context &context, cl_command_queue &queue, cl_program &program, char *sourceFileName);
cl_int initKernels(cl_program program);
void calcSqrt2();
void checks();
void profile();

int _tmain(int argc, _TCHAR* argv[])
{
	using std::cout; using std::endl;
	// init cl
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_int error;

	error = oclInit(context, queue, program, "oclSqrt.cl");
	initKernels(program);

	oclBigInt::context = context;
	oclBigInt::queue = queue;

	//calcSqrt2();
	//checks();
	profile();

	std::cout << "waiting for input to quit." << std::endl;
	std::cin.get();

	return 0;
}

void calcSqrt2() {
	using std::cout; using std::endl;

	const bool useFile = false;

	std::ofstream fOut;
	if (useFile) fOut.open("x.txt");

	DWORD sT = timeGetTime();

	oclBigInt x = 0.7;
	oclBigInt add = 3U;
	add >>= 1;
	//DWORD sT = timeGetTime();
	int lastOkLimbs;
	for (int i = 0; i < 40; i++) {
		//if (i > 2 && i % 2 == 0) {
			//cout << "verify iteration " << i << endl;
		//} else {
			cout << "iteration " << i << endl;
		//}
		if (useFile) fOut << "verify iteration " << i << endl << endl;
		oclBigInt t;
		x.copy(t);
		if (useFile) fOut << "t = x  : " << t << endl << endl;

		t *= x;
		if (useFile) fOut << "t *= x : " << t << endl << endl;

		t.setNeg();
		if (useFile) fOut << "t = ~t : " << t << endl << endl;

		t += add;
		if (useFile) fOut << "t += add : " << t << endl << endl;

		x *= t;
		if (useFile) fOut << "x *= t : " << x << endl << endl;

		const int startResize = 7;
		if (i == startResize) {
			x.verify();
			lastOkLimbs = x.getNumLimbs();
		} else if (i > startResize) {
			lastOkLimbs *= 2;
			x.resize(lastOkLimbs);
		}

		//if (i > 2 && i % 2 == 0) {
		//	x.verify();
		//}

		//oclBigInt b;
		//x.copy(b);
		//b.verify();
		//std::cout << b.getNumLimbs() << "/" << x.getNumLimbs() << " digits." << endl << endl;

		if (i > startResize) {
			//x.verify();
			//if (useFile) fOut << "verified : " << x << endl << endl;
			//oclBigInt b;
			//x.copy(b);
			//b.verify();
			double dT = (double)(timeGetTime() - sT) / 1000.0;
			cout << dT << "s cur limbs : " << x.getNumLimbs() << endl << endl;
			//cout << dT << "s num ok limbs : " << b.getNumLimbs() << endl << endl;
		}

		if (useFile) fOut << "************" << endl << endl;
	}
	x.verify();
	double t = (double)(timeGetTime() - sT) / 1000.0;

	if (useFile) fOut.close();

	x <<= 1;
	cout << "final num limbs : " << x.getNumLimbs() << endl;
	cout << t << "s" << endl;

	//fOut.open("x.txt");
	//fOut << x << endl;
	//fOut.close();
	//c.verify();
	//cout << "sqrt(2) = " << x << endl;
}

void checks() {
	using std::cout; using std::endl;
	srand(time(0));

	size_t xSize = 344;
	size_t ySize = xSize;
	cout << xSize << " limbs\n";

	for (; true; xSize ++, ySize = xSize) {
		BigInt x(0U, xSize - 1);
		for (unsigned int i = 0; i < xSize; i++) {
			x.set((rand() + (rand() << 16)), i);
		}

		BigInt y(0U, ySize - 1);
		for (unsigned int i = 0; i < ySize; i++) {
			y.set((rand() + (rand() << 16)), i);
		}

		//cout << "x = " << x << endl;

		//BigInt a = x * y;
		//BigInt aC = a;
		//BigInt b = x.mulDigit(y, 0x1f);
		//BigInt bC = b;

		oclBigInt a = x.toOcl();
		a *= y.toOcl();
		BigInt aC = a.toBigInt();
		oclBigInt b = x.toOcl();
		b.mul2(y.toOcl());
		BigInt bC = b.toBigInt();

		//cout << x << endl << a << endl << b << endl;
		cout << xSize << ":" << (aC == bC) << "\t";
	}
}

void profile() {
	using std::cout; using std::endl;

	srand(time(0));
	int runs = 2;
	size_t xSize = 0x4;
	size_t ySize = xSize;
	size_t minSize = 18;
	DWORD sT;
	int numLimbs;
	double dT;
	for (; xSize < 0x10000; xSize *= 2, ySize = xSize) {
		//sT = timeGetTime();
		//for (int curRun = 0; curRun < runs; curRun++) {
		//	oclBigInt x = 0.7;
		//	oclBigInt add = 3U;
		//	add >>= 1;
		//	for (int i = 0; i < 24; i++) {
		//		oclBigInt t;
		//		x.copy(t);
		//		t *= x;
		//		t.setNeg();
		//		t += add;
		//		x *= t;
		//		if (i > 2 && i % 2 == 0) {
		//			x.verify();
		//		}
		//	}
		//	x.verify();
		//	numLimbs = x.getNumLimbs();
		//}
		//dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		//cout << "sqrt(2) : x * y (" << runs << " runs)\t: " << dT << " limbs : " << numLimbs << endl;

		cout << "limbs: " << xSize << endl;

		sT = timeGetTime();
		for (int curRun = 0; curRun < runs; curRun++) {
			BigInt x(0U, xSize - 1);
			for (unsigned int i = 0; i < xSize; i++) {
				x.set((rand() + (rand() << 16)), i);
			}

			BigInt y(0U, ySize - 1);
			for (unsigned int i = 0; i < ySize; i++) {
				y.set((rand() + (rand() << 16)), i);
			}

			x.toOcl() *= y.toOcl();
		}
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "v1 : x * y (" << runs << " runs)\t: " << dT << endl;

		sT = timeGetTime();
		for (int curRun = 0; curRun < runs; curRun++) {
			BigInt x(0U, xSize - 1);
			for (unsigned int i = 0; i < xSize; i++) {
				x.set((rand() + (rand() << 16)), i);
			}

			BigInt y(0U, ySize - 1);
			for (unsigned int i = 0; i < ySize; i++) {
				y.set((rand() + (rand() << 16)), i);
			}

			x.toOcl().mul2(y.toOcl());
		}
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "v2 : x * y (" << runs << " runs)\t: " << dT << endl;

		cout << endl;
	}
}

void CL_CALLBACK contextError(const char *errInfo, const void *private_info, size_t cb, void *user_data) {
	std::cout << errInfo << std::endl;
}

cl_int getPlatform(cl_platform_id &_platformId) {
	const int maxPlatforms = 2;
	cl_uint numPlatforms;
	cl_int error;

	// count platforms
	error = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (error != CL_SUCCESS) {
		printf("Error counting platforms: %i", error);
		return error;
	}
	if (numPlatforms > maxPlatforms) {
		printf("maxPlatforms too small for %i platforms.", numPlatforms);
		return 6;
	}

	// fetch platforms
	cl_platform_id platforms[2]; // WARNING: HARD CODE HERE
	error = clGetPlatformIDs(numPlatforms, platforms, NULL);
	if (error != CL_SUCCESS) {
		printf("Error getting platforms: %i", error);
		return error;
	}

	// get platform info
	char buffer[100];
	for (cl_uint i = 0; i < numPlatforms; i++) {
		error = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(buffer), buffer, NULL);
		if (error != CL_SUCCESS) {
			printf("Error getting platform %i info: %i", i, error);
			return error;
		}
	}

	// return
	_platformId = platforms[1];
	return CL_SUCCESS;
}

cl_int getDevice(cl_device_id &_deviceId, const cl_platform_id &_platformId) {
	const int maxDevices = 2;
	cl_int error;
	cl_uint numDevices;

	// count devices
	error = clGetDeviceIDs(_platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
	if (error != CL_SUCCESS) {
		printf("Error counting devices: %i", error);
		return error;
	}
	if (numDevices > maxDevices) {
		printf("maxDevices too small for %i devices.", numDevices);
		return 6;
	}

	// fetch devices
	cl_device_id devices[maxDevices]; // WARNING: HARD CODE HERE
	if (numDevices > maxDevices) { printf("major problem"); }
	error = clGetDeviceIDs(_platformId, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	if (error != CL_SUCCESS) {
		printf("Error fetching devices: %i", error);
		return error;
	}

	// get device info
	char buffer[100];
	for (unsigned int i = 0; i < numDevices; i++) {
		error = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);
		printf("device:%s\n", buffer);
		if (error != CL_SUCCESS) {
			printf("Error getting device %i info: %i", i, error);
			return error;
		}
	}

	// return
	_deviceId = devices[0];
	return CL_SUCCESS;
}

cl_int getContext(cl_context &_context, cl_device_id *_devices, cl_uint _numDevices) {
	cl_int error;
	_context = clCreateContext(0,_numDevices, _devices, contextError, NULL, &error);
	if (error != CL_SUCCESS) {
		printf("Error creating context: %i", error);
		return error;
	}
	return CL_SUCCESS;
}

cl_int getQueue(cl_command_queue &_queue, const cl_context &_context, const cl_device_id &_deviceId) {
	cl_int error;
	_queue = clCreateCommandQueue(_context, _deviceId, 0, &error);
	if (error != CL_SUCCESS) {
		printf("Error creating command queue: %i", error);
		return error;
	}
	return CL_SUCCESS;
}

cl_int oclInit(cl_context &context, cl_command_queue &queue, cl_program &program, char *sourceFileName) {
	cl_platform_id platformId;
	cl_device_id deviceId;
	cl_int error;

	error = getPlatform(platformId);
	if (error != CL_SUCCESS) {
		std::cin.get();
		return error;
	}
	printf("Got platform.\n");

	error = getDevice(deviceId, platformId);
	if (error != CL_SUCCESS) {
		std::cin.get();
		return error;
	}
	printf("Got device.\n");

	error = getContext(context, &deviceId, 1);
	if (error != CL_SUCCESS) {
		std::cin.get();
		return error;
	}
	printf("Got context.\n");

	error = getQueue(queue, context, deviceId);
	if (error != CL_SUCCESS) {
		std::cin.get();
		return error;
	}
	printf("Got queue.\n");

	// create program
	std::ifstream in(sourceFileName, std::ios::binary);
	std::string source;
	in.seekg(0, std::ios::end);
	source.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(&source[0], source.size());
	in.close();
	printf("Source loaded.\n");
	const char *buffer = source.c_str();
	const size_t sourceSize = source.size();
	program = clCreateProgramWithSource(context, 1, &buffer, &sourceSize, &error);
	if (error != CL_SUCCESS) {
		printf("Error loading program: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program created.\n");

	// build program
	error = clBuildProgram(program, 1, &deviceId, "", NULL, NULL);

	// show build log
	char *buildLog;
	size_t logSize;

	// count build log
	clGetProgramBuildInfo(program, deviceId, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
	buildLog = new char[logSize+1];
	clGetProgramBuildInfo(program, deviceId, CL_PROGRAM_BUILD_LOG, logSize, buildLog, NULL);
	buildLog[logSize] = 0;
	printf("%s", buildLog);
	delete[] buildLog;

	if (error != CL_SUCCESS) {
		printf("Error building program: %i", error);
		std::cin.get();
		return error;
	}

	//cl_ulong maxMemAlloc;
	//clGetDeviceInfo(deviceId, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &maxMemAlloc, NULL);
	//printf("CL_DEVICE_MAX_MEM_ALLOC_SIZE : %u\n", maxMemAlloc);

	return CL_SUCCESS;
}

cl_int initKernels(cl_program program) {
	cl_int error;

	// get kernel
	oclBigInt::fillKernel = clCreateKernel(program, "fill", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::shlKernel = clCreateKernel(program, "shl", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::shrKernel = clCreateKernel(program, "shr", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::addKernel = clCreateKernel(program, "add", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::carryKernel = clCreateKernel(program, "carry", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::negKernel = clCreateKernel(program, "neg", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::mulKernel = clCreateKernel(program, "mul", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::rmaskKernel = clCreateKernel(program, "rmask", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::countKernel = clCreateKernel(program, "count", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::mul2Kernel = clCreateKernel(program, "mul2", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");

	oclBigInt::carry2Kernel = clCreateKernel(program, "carry2", &error);
	if (error != CL_SUCCESS) {
		printf("Error creating kernel: %i", error);
		std::cin.get();
		return error;
	}
	printf("Program built.\n");
}