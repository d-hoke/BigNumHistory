// oclSqrt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BigInt.h"
#include "oclBigInt.h"
#include "test.h"

//#define USE_FILE
//#define DEBUG_SQRT


cl_int getPlatform(cl_platform_id &_platformId);
cl_int getDevice(cl_device_id &_deviceId, const cl_platform_id &_platformId);
cl_int getContext(cl_context &_context, cl_device_id *_devices, cl_uint _numDevices);
cl_int getQueue(cl_command_queue &_queue, const cl_context &_context, const cl_device_id &_deviceId);
cl_int oclInit(cl_context &context, cl_command_queue &queue, cl_program &program, char *sourceFileName);
cl_int initKernels(cl_program program);
void calcSqrt2();
void checks();
void test();
void profileCPU();
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
	oclBigInt::init();

	//calcSqrt2();
	//checks();
	test();
	//profile();
	//profileCPU();

	std::cout << "waiting for input to quit." << std::endl;
	std::cin.get();

	oclBigInt::close();

	return 0;
}

void calcSqrt2() {
	using std::cout; using std::endl;

#ifdef USE_FILE
	std::ofstream fOut;
	fOut.open("x.txt");
#endif

	DWORD sT = timeGetTime();

	oclBigInt x = 0.7;
	oclBigInt add = 3;
#ifdef DEBUG_SQRT
	BigInt cadd = add.toBigInt();
	cadd = cadd >> 1;
#endif
	add >>= 1;
	int lastOkLimbs;
	oclBigInt::resetProfiling();
	for (int i = 0; i < 30; i++) {
		cout << "iteration " << i << endl;
		oclBigInt t;
		x.copy(t);
		t *= x;

#ifdef USE_FILE
		fOut << "verify iteration " << i << endl << endl;
		fOut << "x =    : " << x << endl << endl;
		fOut << "t *= x : " << t << endl << endl;
#endif
#ifdef DEBUG_SQRT
		BigInt ct = x.toBigInt();
		ct = ct * ct;
		if (!(t.toBigInt() == ct)) {
			std::cout << "stop.";
		}
#endif

		t.setNeg();

#ifdef USE_FILE
		fOut << "t = ~t : " << t << endl << endl;
#endif
#ifdef DEBUG_SQRT
		ct.setNeg();
		if (!(t.toBigInt() == ct)) {
			std::cout << "stop.";
		}
#endif

		t += add;

#ifdef USE_FILE
		fOut << "t += add : " << t << endl << endl;
#endif
#ifdef DEBUG_SQRT
		ct += add.toBigInt();
		if (!(t.toBigInt() == ct)) {
			std::cout << "stop.";
		}
		BigInt cx = x.toBigInt();
		cx = cx * ct;
#endif
		x *= t;

#ifdef DEBUG_SQRT
		if (!(x.toBigInt() == cx)) {
			std::cout << "stop.";
		}
#endif
#ifdef USE_FILE
		fOut << "x *= t : " << x << endl << endl;
#endif

		const int startResize = 7;
		if (i == startResize) {
			x.verify();
			lastOkLimbs = x.getNumLimbs();
		} else if (i > startResize) {
			lastOkLimbs *= 2;
			x.resize(lastOkLimbs);
		}

		if (i > startResize) {
			double dT = (double)(timeGetTime() - sT) / 1000.0;
			cout << dT << "s cur limbs : " << x.getNumLimbs() << endl;
			oclBigInt::printProfiling(1, dT);
			oclBigInt::resetProfiling();
			cout << endl;
		}

#ifdef USE_FILE
		fOut << "************" << endl << endl;
#endif

	}
	x.verify();
	double t = (double)(timeGetTime() - sT) / 1000.0;

#ifdef USE_FILE
	fOut.close();
#endif

	x <<= 1;
	cout << "final num limbs : " << x.getNumLimbs() << endl;
	cout << t << "s" << endl;
}

void checks() {
	using std::cout; using std::endl;
	srand((unsigned int)time(0));

	size_t szX = 0x1000;
	size_t szY = szX;
	cout << szX << " limbs\n";
	bool read = true;

	BigInt x, y;
	if (read) {
		//x.in("c_1.txt");
		x.in("x.txt");
		szX = x.numLimbs();
		//y.in("ct.txt");
		y.in("y.txt");
		szY = y.numLimbs();
	}
	for (; true; ) {
		//BigInt x(0U, xSize - 1);
		if (!read) {
			x.limbs.resize(szX);
			for (unsigned int i = 0; i < szX; i++) {
				x.set((rand() + (rand() << 16)), i);
				x.set(0xffffffff, i);
			}
			//x.set(0x0b91747b, 0);
			//x.set(0x54a848ee, 1);
			y.limbs.resize(szY);
			for (unsigned int i = 0; i < szY; i++) {
				y.set((rand() + (rand() << 16)), i);
				//y.set(0xffffffff, i);
			}
		}
		//int d = rand() % (szX * 32);

		//BigInt a = x.baseMul(y);
		//BigInt a = x << d;
		//BigInt a = x + y;
		//BigInt aC = a;
		//BigInt b = x.baseMul(y);
		//BigInt bC = b;

		oclBigInt oX = x;
		oclBigInt oY = y;

		//oclBigInt a;
		//oX.copy(a);
		//a.mul2(oY);
		//a.baseMul(oY);
		//BigInt aC = a.toBigInt();

		oclBigInt b;
		oX.copy(b);
		b.baseMul(oY);
		//b.mul2(oY);
		b.truncate();
		//b += oY;
		//b.mul2(oY);
		//b <<= d;
		//BigInt bC = b.toBigInt();

		//bool ok = aC == bC;
		bool ok = true;
		//if (!read && !ok) {
			//x.out("x.txt");
			//y.out("y.txt");
		//}

		//bool ok = true;
		cout << szY << ":" << ok << "\n";
	}
}

void profileCPU() {
	using std::cout; using std::endl;

	srand((unsigned int)time(0));

	const int runs = 0x2;
	size_t xSize = 0x800;
	size_t ySize = xSize;
	DWORD sT;
	double dT;

	for (; xSize < 0x10000000; xSize *= 2, ySize = xSize) {
		//oclBigInt::resetProfiling();
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

			x.mulDigit(y);
		}
		//clFlush(oclBigInt::queue);
		//clFinish(oclBigInt::queue);
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "cpu : x mulDigit y (" << runs << " runs, " << xSize << " limbs)\t: " << dT << endl;
		//oclBigInt::printProfiling(runs);
	}
}

void profile() {
	using std::cout; using std::endl;

	srand((unsigned int)time(0));
	int runs = 0x400;
	size_t xSize = 0x800;
	//size_t xSize = 0x200000;
	size_t ySize = xSize;
	size_t startSize = 0x6000;
	size_t endSize = 69632;
	size_t minSize = startSize;
	DWORD sT;
	//int numLimbs;
	double dT;
	for (; xSize < 0x10000000; xSize *= 2, ySize = xSize) {
		//sT = timeGetTime();
		//for (int curRun = 0; curRun < runs; curRun++) {
		//	oclBigInt x = 0.7;
		//	oclBigInt add = 3U;
		//	add >>= 1;
		//	for (int i = 0; i < 24; i++) {
				//oclBigInt t;
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

		//cout << "limbs : " << xSize << " minSize : " << minSize << endl;

		BigInt x(0U, xSize - 1);
		for (unsigned int i = 0; i < xSize; i++) {
			x.set((rand() + (rand() << 16)), i);
		}

		BigInt y(0U, ySize - 1);
		for (unsigned int i = 0; i < ySize; i++) {
			y.set((rand() + (rand() << 16)), i);
		}
		oclBigInt oX = x;
		oclBigInt oY = y;
		oclBigInt tX(oX.getNumLimbs());
		oclBigInt tY(oY.getNumLimbs());
		//BigInt tX(0, x.numLimbs() - 1);
		//BigInt tY(0, y.numLimbs() - 1);

		oclBigInt::resetProfiling();
		sT = timeGetTime();
		for (int curRun = 0; curRun < runs; curRun++) {
			//tX = x;
			//tX.baseMul(y);

			//x * y;
			oX.copy(tX);
			oY.copy(tY);
			//oclBigInt::fillBuffer(tX.limbs, 0, 0, tX.numLimbs);
			//tX.addCarry(tX.limbs, tY.limbs, tY.getNumLimbs());
			//tX += oY;
			//tX.baseMul(tY);
			tX.mul2(tY, 0xc000);
			//tX.mul2(tY, 0xc000);
			//x.mulDigit(y, 0x200);
		}
		clFlush(oclBigInt::queue);
		clFinish(oclBigInt::queue);
		dT = (double)(timeGetTime() - sT) / 1000.0 / (double)runs;
		cout << "v1 : x * y (" << runs << " runs, " << xSize << " limbs, " << minSize << " minSize)\t: " << dT << endl;
		oclBigInt::printProfiling(runs, dT);

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
	_context = clCreateContext(0,_numDevices, _devices, NULL, NULL, &error);
	//_context = clCreateContext(0,_numDevices, _devices, contextError, NULL, &error);
	if (error != CL_SUCCESS) {
		printf("Error creating context: %i", error);
		return error;
	}
	return CL_SUCCESS;
}

cl_int getQueue(cl_command_queue &_queue, const cl_context &_context, const cl_device_id &_deviceId) {
	cl_int error;
	_queue = clCreateCommandQueue(_context, _deviceId, CL_QUEUE_PROFILING_ENABLE, &error);
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
	source.resize((int)in.tellg());
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
	oclBigInt::safeCL(error, "create kernels", "fill");
	printf("Program built.\n");

	oclBigInt::shlKernel = clCreateKernel(program, "shl", &error);
	oclBigInt::safeCL(error, "create kernels", "shl");
	printf("Program built.\n");

	oclBigInt::shrKernel = clCreateKernel(program, "shr", &error);
	oclBigInt::safeCL(error, "create kernels", "shr");
	printf("Program built.\n");

	oclBigInt::addKernel = clCreateKernel(program, "add", &error);
	oclBigInt::safeCL(error, "create kernels", "add");
	printf("Program built.\n");

	oclBigInt::negKernel = clCreateKernel(program, "neg", &error);
	oclBigInt::safeCL(error, "create kernels", "neg");
	printf("Program built.\n");

	oclBigInt::rmaskKernel = clCreateKernel(program, "rmask", &error);
	oclBigInt::safeCL(error, "create kernels", "rmask");
	printf("Program built.\n");

	oclBigInt::countKernel = clCreateKernel(program, "count", &error);
	oclBigInt::safeCL(error, "create kernels", "count");
	printf("Program built.\n");

	oclBigInt::mul2Kernel = clCreateKernel(program, "mul2", &error);
	oclBigInt::safeCL(error, "create kernels", "mul2");
	printf("Program built.\n");

	//oclBigInt::carry2Kernel = clCreateKernel(program, "carry2", &error);
	//oclBigInt::safeCL(error, "create kernels", "carry2");
	//printf("Program built.\n");

	oclBigInt::carryOneKernel = clCreateKernel(program, "carryOne", &error);
	oclBigInt::safeCL(error, "create kernels", "carryOne");
	printf("Program built.\n");

	oclBigInt::shortAddOffKernel = clCreateKernel(program, "shortAddOff", &error);
	oclBigInt::safeCL(error, "create kernels", "shortAddOff");
	printf("Program built.\n");

	oclBigInt::checkAddKernel = clCreateKernel(program, "checkAdd", &error);
	oclBigInt::safeCL(error, "create kernels", "checkAdd");
	printf("Program built.\n");

	oclBigInt::toggleKernel = clCreateKernel(program, "toggle", &error);
	oclBigInt::safeCL(error, "create kernels", "toggle");
	printf("Program built.\n");

	return error;
}