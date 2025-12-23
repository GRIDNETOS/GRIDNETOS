#include "CFSResult.h"

CFSResult::CFSResult(bool success, const std::vector<uint8_t>& data, const  std::string& error) {
    this->success = success;
    this->data = data;
    this->error = error;
    this->mDataType = eDataType::bytes;
}

CFSResult::~CFSResult() {
}

bool CFSResult::getSuccess() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFieldsGuardian));
    return success;
}

void CFSResult::setSuccess(bool success) {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFieldsGuardian));
    this->success = success;
}

std::vector<uint8_t> CFSResult::getData() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFieldsGuardian));
    return data;
}

void CFSResult::setData(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    this->data = data;
}

eDataType::eDataType CFSResult::getDataType()
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    return mDataType;
}

void CFSResult::setDataType(eDataType::eDataType dType)
{
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    mDataType = dType;
}

std::string CFSResult::getError() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mFieldsGuardian));
    return error;
}

void CFSResult::setError(const std::string& error) {
    std::lock_guard<std::mutex> lock(mFieldsGuardian);
    this->error = error;
}
