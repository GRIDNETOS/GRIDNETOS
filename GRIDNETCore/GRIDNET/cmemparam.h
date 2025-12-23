#ifndef CMEMPARAM_H
#define CMEMPARAM_H

#include "../interfaces/imemparam.h"

class CMemParam : public IMemParam
{
public:
    CMemParam(eParamType paramType, eMemType memType, eAccessType accessType, uint64_t memSize);
};

#endif // CMEMPARAM_H
