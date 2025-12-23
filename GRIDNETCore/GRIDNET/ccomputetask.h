#ifndef CCOMPUTETASK_H
#define CCOMPUTETASK_H

#include "../interfaces/icomputetask.h"

class CComputeTask : public IComputeTask
{
public:
    CComputeTask(const std::string &openCLSourceFile, const std::string &name);
};

#endif // CCOMPUTETASK_H
