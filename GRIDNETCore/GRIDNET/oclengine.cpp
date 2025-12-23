#include "oclengine.h"
#include <stdio.h>
#include <stdlib.h>
#include "BlockchainManager.h"
#include "Settings.h"
#include <iostream>
#include "arith_uint256.h"
#include "Worker.h"
#include <regex>
#include <set>
#include <array>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"
#endif

// CL_DEVICE_UUID_KHR from cl_khr_device_uuid extension
#ifndef CL_DEVICE_UUID_KHR
#define CL_DEVICE_UUID_KHR 0x106A
#endif
#ifndef CL_UUID_SIZE_KHR
#define CL_UUID_SIZE_KHR 16
#endif

int num = 0;
#define CL_SET_ARG(var) status |= clSetKernelArg(*kernel, num++, sizeof(var), (void *)&var)
typedef unsigned int       uint32_t;
#define	bswap_16(value)  \
 	((((value) & 0xff) << 8) | ((value) >> 8))
#define	bswap_32(value)	\
 	(((uint32_t)bswap_16((unsigned short)((value) & 0xffff)) << 16) | \
 	(uint32_t)bswap_16((unsigned short)((value) >> 16)))


static inline unsigned int swab32(unsigned int v)
{
	return bswap_32(v);
}


static inline void flip80(void *dest_p, const void *src_p)
{
	unsigned int *dest = (unsigned int *)dest_p;
	const unsigned int  *src = (unsigned int *)src_p;
	int i;

	for (i = 0; i < 20; i++)
		dest[i] = swab32(src[i]);
}
enum alive {
	LIFE_WELL,
	LIFE_SICK,
	LIFE_DEAD,
	LIFE_NOSTART,
	LIFE_INIT,
};




bool COCLEngine::getWasInitialised()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mInitialised;
}

COCLEngine::~COCLEngine()
{
	for (int i = 0; i < mWorkers.size(); i++)
	{
		mWorkers[i].reset();
    }

	cl_int ret;
	if (mProgram != NULL)
	{
		ret = clReleaseProgram(mProgram);
		mProgram = NULL;
	}

	for (int i = 0; i < mContexts.size(); i++)
	{
	if(mContexts[i]!=NULL)
	{
	ret = clReleaseContext(mContexts[i]);
	mContexts[i] = NULL;
	}
}
}

COCLEngine::COCLEngine()
{
	mTools = std::make_unique<CTools>("OCL Engine", eBlockchainMode::LocalData);

	mInitialised = false;
	mProgram = nullptr;
	mRetNumPlatforms = 0;
}

bool COCLEngine::Initialize(bool useCPU , bool useGPU )
{

	std::shared_ptr<CSettings>  set = CSettings::getInstance(eBlockchainMode::LocalData);
	try {
		if (mInitialised)
		{
			mTools->writeLine("The computational platform has been initialized already.", true, true, eViewState::unspecified, "OCL Engine");
			return true;
		}
		mTools->writeLine("Attempting to initialize the decentralized computations engine..", true, true, eViewState::unspecified, "OCL Engine");

		if (useCPU == false && useGPU == false)
		{
			useGPU = set->getUseGPUs();
			if (useGPU)
			{

				//while (mTools->getCurrentTDRValue() < 10)
				//{
				//	mTools->askYesNo("GPU mining enabled while the Windows GPU driver's TDR value needs to be equal to at least 10. \
				//		Restart the software with Administrative privilages to tweak the value automatically, or set it manually at \
				//		Computer\\HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers. \n Hit Enter to verify again.", true);
				//}

			}
			useCPU = set->getUseCPUs();
		}
		if (useCPU && useGPU)
			mTools->writeLine("Will attempt to use both the available CPUs and GPUs", true, true, eViewState::unspecified, "OCL Engine");
		else if (useCPU)
			mTools->writeLine("Will attempt to use only the available CPUs", true, true, eViewState::unspecified, "OCL Engine");
		
		else if(useGPU)
			mTools->writeLine("Will attempt to use only the available GPUs", true, true, eViewState::unspecified, "OCL Engine");
		else
		{
			mTools->writeLine("I was not to use any CPUs or GPUs. Computational engine won't be available.", true, true, eViewState::unspecified, "OCL Engine");
			return false;
		}
		cl_int ret;
		unsigned int nrOfActiveContexts = 0;
		ret = clGetPlatformIDs(0, NULL, &mRetNumPlatforms);

		if (ret != CL_SUCCESS)
		{
			mTools->writeLine("Error retrieving OpenCL platform data.", true, true, eViewState::unspecified, "OCL Engine");

		}

		if (mRetNumPlatforms > 0)
		{
			if (mRetNumPlatforms > 1)
				mTools->writeLine("Found " + std::to_string(mRetNumPlatforms) + " OpenCL platforms", true, true, eViewState::unspecified, "OCL Engine");
			else
				mTools->writeLine("Found a single OpenCL platform", true, true, eViewState::unspecified, "OCL Engine");
			this->mPlatforms.resize(mRetNumPlatforms);
		}
		else
		{
			mTools->writeLine("No OpenCL platform available. Aborting.");
			return false;
		}

		ret = clGetPlatformIDs(mRetNumPlatforms, mPlatforms.data(), NULL);
		if (ret != CL_SUCCESS)
		{
			mTools->writeLine("Error retrieving OpenCL platform data.", true, true, eViewState::unspecified, "OCL Engine");

		}
		std::vector<cl_device_id> devices;
		std::vector<cl_command_queue> commandQueues;

		cl_context context;
		cl_uint numberOfDevices;
		cl_uint numberOfClones = 1; //"How many times to clone//1==no clonesdata
		//query for available compute platforms
		mTools->writeLine("Processing computational platforms..", true, true, eViewState::unspecified, "OCL Engine");

		uint64_t preferredPlatformsCount = 0;


		mTools->writeLine("", true, true, eViewState::unspecified, "OCL Engine");
		if (mRetNumPlatforms && useGPU)
		{
			mTools->writeLine("[Stage 1]: doing preliminary computational platforms' analysis..", true, true, eViewState::unspecified, "OCL Engine");
			for (int i = 0; i < mRetNumPlatforms; i++)// the order of initialization seems to be important on some intel/nvidia (Optimus) systems.
				//on those systems the Nvidia cards might not get initialized properly; and return invalid computational results.
			{
				size_t platformNameSize = 0;
				clGetPlatformInfo(mPlatforms[i], CL_PLATFORM_NAME, 0, NULL, &platformNameSize);
				std::vector< char > szPlatformName(platformNameSize);
				szPlatformName.resize(platformNameSize);
				clGetPlatformInfo(mPlatforms[i], CL_PLATFORM_NAME, platformNameSize, szPlatformName.data(), &platformNameSize);
				std::string platformName = std::string(szPlatformName.begin(), szPlatformName.end());
				mTools->writeLine("Pre-Processing platform: " + platformName, true, true, eViewState::unspecified, "OCL Engine");
				if (set->getUsePlatformID(platformName))
				{
					mTools->writeLine("Platform " + platformName +mTools->getColoredString(" is preferred.",eColor::lightGreen), true, true, eViewState::unspecified, "OCL Engine");
					preferredPlatformsCount++;
				}
				else
				{
					mTools->writeLine("Platform " + platformName + mTools->getColoredString(" is not preferred..", eColor::lightPink), true, true, eViewState::unspecified, "OCL Engine");
				}

			}
			if (preferredPlatformsCount)
			{
				mTools->writeLine(mTools->getColoredString("Found " + std::to_string(preferredPlatformsCount) + " preferred platforms..", eColor::lightGreen) + " I'll more picky and neglect integrated GPU platforms..", true, true, eViewState::unspecified, "OCL Engine");
			}
			else 
			{
				mTools->writeLine(mTools->getColoredString("I haven't found any preferred platforms..", eColor::lightPink) + ". I'll use the remaining ones if any..", true, true, eViewState::unspecified, "OCL Engine");
			}
		}

		// Track device UUIDs to prevent duplicate device registration across platforms
		// This handles cases where the same physical GPU appears under multiple OpenCL platforms
		// (e.g., NVIDIA registering the same GPUs under two separate CUDA platforms)
		std::set<std::array<unsigned char, CL_UUID_SIZE_KHR>> seenDeviceUUIDs;

		if (mRetNumPlatforms)
		{
			mTools->writeLine("[Stage 2]: doing main computational platforms' analysis..", true, true, eViewState::unspecified, "OCL Engine");

			// Platforms - BEGIN

			for (int i = 0; i < mRetNumPlatforms; i++)// the order of initialization seems to be important on some Intel/NVIDIA (Optimus) systems.
				//on those systems the NVIDIA cards might not get initialized properly; and return invalid computational results.
			{
				size_t platformNameSize = 0;
				clGetPlatformInfo(mPlatforms[i], CL_PLATFORM_NAME, 0, NULL, &platformNameSize);
				std::vector< char > szPlatformName(platformNameSize);
				szPlatformName.resize(platformNameSize);
				clGetPlatformInfo(mPlatforms[i], CL_PLATFORM_NAME, platformNameSize, szPlatformName.data(), &platformNameSize);
				std::string platformName = std::string(szPlatformName.begin(), szPlatformName.end());
				mTools->writeLine("Processing platform: " + platformName, true, true, eViewState::unspecified, "OCL Engine");
				if (preferredPlatformsCount && !set->getUsePlatformID(platformName))
					continue;


				bool error = false;
				numberOfDevices = 0;
				devices.clear();
				context = NULL;
				cl_device_type deviceTypes = CL_DEVICE_TYPE_ALL;
				bool fallbackPerformed = false;
			fallbackDetection:
				if (useCPU && useGPU)
				{
					mTools->writeLine("looking for GPUs as wall as CPUs..", true, true, eViewState::unspecified, "OCL Engine");
					deviceTypes = CL_DEVICE_TYPE_ALL;
				}
				else if (useCPU)
				{
					mTools->writeLine("looking for CPUs only..", true, true, eViewState::unspecified, "OCL Engine");
					deviceTypes = CL_DEVICE_TYPE_CPU;
				}
				else if (useGPU)
				{
					mTools->writeLine("looking for GPUs only..", true, true, eViewState::unspecified, "OCL Engine");
					deviceTypes = CL_DEVICE_TYPE_GPU;
				}

				ret = clGetDeviceIDs(mPlatforms[i], deviceTypes, 0, NULL, &numberOfDevices);
				if (numberOfDevices > 0)
				{
					devices.resize(numberOfDevices);
					ret = clGetDeviceIDs(mPlatforms[i], deviceTypes,
						numberOfDevices, devices.data(), NULL);
					mTools->writeLine("Found " + std::to_string(numberOfDevices), true, true, eViewState::unspecified, "OCL Engine");
				}
				else
				{
					mTools->writeLine(mTools->getColoredString("No preferred devices found on this platform.", eColor::lightPink), true, true, eViewState::unspecified, "OCL Engine");
					if (!fallbackPerformed)
					{
						mTools->writeLine(mTools->getColoredString("Falling back to any CPU/GPU computational devices..", eColor::orange), true, true, eViewState::unspecified, "OCL Engine");
						fallbackPerformed = true;
						useCPU = true;
						useGPU = true;
						goto fallbackDetection;
					}

					continue;
				}
				
				//query device properties create Workers
				size_t ret_size;
				cl_uint compute_units;
				cl_ulong max_alloc;
				size_t max_work_size;
				std::string name;
				std::vector<char> c_name;
				//int b = 0;
				uint32_t retrieved;
				CSettings::getInstance(eBlockchainMode::LocalData)->getNrOfClonesForPlatform(platformName, retrieved);
				numberOfClones += retrieved; // mTools->askInt("How many clones to create (1-20)? ", 0);

				if (numberOfClones <= 0 || numberOfClones > 20)
				{
					numberOfClones = 1;
					mTools->writeLine("Invalid number of clones. None will be created.", true, true, eViewState::unspecified, "OCL Engine");
				}

				if (numberOfClones > 1)
					mTools->writeLine("Each " + platformName + " device will be cloned " + std::to_string(numberOfClones - 1) + " times.", true, true, eViewState::unspecified, "OCL Engine");
				int resulting = 0;

				// Devices - BEGIN
				for (int y = 0; y < numberOfDevices; y++)
				{
					// Device UUID deduplication - check if this physical device was already registered
					// from another platform (common with NVIDIA registering same GPUs under multiple CUDA platforms)
					std::array<unsigned char, CL_UUID_SIZE_KHR> deviceUUID = {};
					cl_int uuidRet = clGetDeviceInfo(devices[y], CL_DEVICE_UUID_KHR, CL_UUID_SIZE_KHR, deviceUUID.data(), NULL);

					if (uuidRet == CL_SUCCESS)
					{
						if (seenDeviceUUIDs.find(deviceUUID) != seenDeviceUUIDs.end())
						{
							// This device UUID was already seen - skip duplicate
							size_t dupNameSize = 0;
							clGetDeviceInfo(devices[y], CL_DEVICE_NAME, 0, NULL, &dupNameSize);
							std::vector<char> dupName(dupNameSize);
							clGetDeviceInfo(devices[y], CL_DEVICE_NAME, dupNameSize, dupName.data(), NULL);
							mTools->writeLine(mTools->getColoredString("Skipping duplicate device: ", eColor::orange) +
								std::string(dupName.begin(), dupName.end()) + " (already registered from another platform)",
								true, true, eViewState::unspecified, "OCL Engine");
							continue;
						}
						seenDeviceUUIDs.insert(deviceUUID);
					}
					// If UUID query fails (extension not supported), fall through and register the device
					// This maintains backward compatibility with older drivers/hardware

					//we now create a context for each device separately to increase compartmentalization levels.
					context = clCreateContext(NULL, 1, &devices[y], NULL, NULL, &ret);
					if (ret != CL_SUCCESS)
						throw(std::abort);
					mContexts.push_back(context);
					if (ret != CL_SUCCESS)
					{
						error = true;
					}

					std::vector<uint8_t> groupID = mTools->genRandomVector(32);
					for (int b = 0; b < numberOfClones; b++)
					{
						ret_size = compute_units = max_alloc = max_work_size = 0;
						c_name.clear();

						ret = clGetDeviceInfo(devices[y], CL_DEVICE_NAME, NULL, NULL, &ret_size);
						if (ret != CL_SUCCESS)
						{
							error = true; goto errored;
						}
						c_name.resize(ret_size);
						ret = clGetDeviceInfo(devices[y], CL_DEVICE_NAME, c_name.size(), c_name.data(), &ret_size);
						if (ret != CL_SUCCESS)
						{
							error = true; goto errored;
						}
						name = std::string(c_name.begin(), c_name.end());
						name = std::regex_replace(name, std::regex("[' ']{2,}"), " ");

						cl_device_type   devType;
						ret = clGetDeviceInfo(devices[y], CL_DEVICE_TYPE, sizeof(cl_device_type), (void*)&devType, NULL);
						if (ret != CL_SUCCESS)
						{
							error = true; goto errored;
						}

						ret = clGetDeviceInfo(devices[y], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), (void*)&compute_units, NULL);
						if (ret != CL_SUCCESS)
						{
							error = true; goto errored;
						}
						ret = clGetDeviceInfo(devices[y], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), (void*)&max_work_size, NULL);
						if (ret != CL_SUCCESS)
						{
							error = true; goto errored;
						}
						CWorker::eWorkerType type;
						if (devType & CL_DEVICE_TYPE_GPU)
							type = CWorker::eWorkerType::GPU;
						else
							if (devType & CL_DEVICE_TYPE_CPU)
								type = CWorker::eWorkerType::CPU;

						ret = clGetDeviceInfo(devices[y], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), (void*)&max_alloc, NULL);
						if (ret != CL_SUCCESS)
						{
							error = true;
							goto errored;
						}
					errored:
						if (error != true)
						{
							std::shared_ptr<CWorker>  w = std::make_shared <CWorker>(groupID);
							cl_command_queue cc = clCreateCommandQueue(context, devices[y], 0, &ret);
							if (ret != CL_SUCCESS)
							{
								error = true;
								goto errored;
							}
							w->setContext(context);
							w->setDevice(devices[y]);
							w->setCommandQueue(cc);
							w->setMaxComputeUnits(compute_units);
							w->setMaxMemAlloc(max_alloc / numberOfClones);
							w->setMaxWorkGroupSize(max_work_size);
							w->setNamePrefix(name + (numberOfClones > 0 ? " (cloned)" : ""));
							std::cmatch cm;
							if (std::regex_search(name.data(), cm, std::regex("\\w\+")))
								w->setShortName(std::string(cm[0]) + "-" + std::to_string(mWorkers.size() + 1));
							

							w->setType(type);
							resulting++;
							mWorkers.push_back(w);
						}
					}
				}
				// Devices - END

				mTools->writeLine("Initialized " + std::to_string(resulting) + " " + platformName + " devices.", true, true, eViewState::unspecified, "OCL Engine");

				nrOfActiveContexts++;
			}

			// Platforms - END

		}

		if (mWorkers.size() > 0)
		{
			mInitialised = true;
			mTools->writeLine("OpenCL initialization succeeded.", true, true, eViewState::unspecified, "OCL Engine");
		}

		else
		{
			mInitialised = false;
			mTools->writeLine("OpenCL initialization failed. No OpenCL workers were initialized.", true, true, eViewState::unspecified, "OCL Engine");
		}

		return mInitialised;
	}
	catch (std::bad_alloc ex)
	{
		CTools::lowMemShutdown();
		return true;
	}
	return true;
	}



std::vector<cl_platform_id>* COCLEngine::getPlatforms()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    if (!mInitialised)
		return nullptr;
    return &mPlatforms;
}




std::vector<std::shared_ptr<CWorker>> COCLEngine::getWorkers()
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
    return mWorkers;
}

std::vector<std::shared_ptr<CWorker>> COCLEngine::getWorkersByGroupID(std::vector<uint8_t> groupID)
{
	std::lock_guard<std::recursive_mutex> lock(mFieldsGuardian);
	std::vector<std::shared_ptr<CWorker>> toRet;
	if (groupID.size() != 32)
		return toRet;

	for (int i = 0; i < mWorkers.size(); i++)
	{
		if (mWorkers[i]->getGroupID().size() == 32)
		{
			if (std::memcmp(mWorkers[i]->getGroupID().data(), groupID.data(), 32))
				toRet.push_back(mWorkers[i]);

		}
	}
	return toRet;
}


