#pragma once
#ifndef CFSRESULT_H
#define CFSRESULT_H

#include <vector>
#include <string>
#include <mutex>
#include "enums.h"

class CFSResult {
public:
    CFSResult(bool success = true, const std::vector<uint8_t>& data = std::vector<uint8_t>(), const std::string &error="");
    ~CFSResult();

    bool getSuccess() const;
    void setSuccess(bool success);

    std::vector<uint8_t> getData() const;
    void setData(const std::vector<uint8_t>& data);
    eDataType::eDataType getDataType();
    void setDataType(eDataType::eDataType dType);
    std::string getError() const;
    void setError(const std::string& error);

private:
    bool success;
    std::vector<uint8_t> data;
    std::string error;
    eDataType::eDataType mDataType;

    std::mutex mFieldsGuardian;

};

#endif // CFSRESULT_H
