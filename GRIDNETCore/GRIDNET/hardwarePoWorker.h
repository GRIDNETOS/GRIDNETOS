#pragma once
#include "stdafx.h"
#include <thread>
#include "powjob.h"

class CHardwarePoWorker
{
private:
	size_t max_work_size;
	size_t wsize;
	size_t compute_shaders;

	bool hasBitAlign;
	bool hasOpenCL11plus;
	bool hasOpenCL12plus;
	cl_mem outputBuffer;
	cl_context context;
	cl_command_queue commandQueue;
	cl_program program;
	cl_mem outputBuffer;
	cl_mem CLbuffer0;
	cl_mem hash_buffer;
public:
};