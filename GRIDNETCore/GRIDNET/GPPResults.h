#pragma once
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
class CGPPResults {
private:
    rapidjson::Document m_doc;
    bool mGotDocument = false;
    std::vector<uint8_t> getValueAsUint8Vector(const rapidjson::Value& value) const {
        std::vector<uint8_t> vec;
        for (auto& v : value.GetArray()) {
            vec.push_back(static_cast<uint8_t>(v.GetInt()));
        }
        return vec;
    }

    std::vector<std::string> getValueAsStringVector(const rapidjson::Value& value) const {
        std::vector<std::string> vec;
        for (auto& v : value.GetArray()) {
            vec.push_back(v.GetString());
        }
        return vec;
    }

    // Helper method to convert value to string
    std::string valueToString(const rapidjson::Value& value) const {
        std::ostringstream ss;
        if (value.IsString()) {
            ss << value.GetString();
        }
        else if (value.IsArray()) {
            ss << "[ ";
            for (auto& v : value.GetArray()) {
                ss << valueToString(v) << " ";
            }
            ss << "]";
        }
        else if (value.IsBool()) {
            ss << (value.GetBool() ? "true" : "false");
        }
        else if (value.IsInt()) {
            ss << value.GetInt();
        }
        else if (value.IsDouble()) {
            ss << value.GetDouble();
        }
        else if (value.IsUint()) {
            ss << value.GetUint();
        }
        else {
            ss << "null";
        }
        return ss.str();
    }

public:
    CGPPResults(const std::string& jsonResults) {
        if (jsonResults.size())
        {
            m_doc.Parse(jsonResults.c_str());
            mGotDocument = true;
        }
        else
        {
            mGotDocument = false;
        }
    }

    bool hasKey(const std::string& key) const {
        return m_doc.HasMember(key.c_str());
    }

    size_t count() const {
        if (!mGotDocument)
        {
            return 0;
        }
        if (m_doc.IsObject())
            return m_doc.MemberCount();
        else if (m_doc.IsArray())
            return m_doc.Size();
        else
            throw std::runtime_error("Invalid type: document is neither an object nor an array.");
    }

    std::multimap<std::string, std::string> getAll() const {
        std::multimap<std::string, std::string> allValues;
        if (!mGotDocument)
        {
            return allValues;
        }
        if (m_doc.IsObject()) {
            for (rapidjson::Value::ConstMemberIterator itr = m_doc.MemberBegin(); itr != m_doc.MemberEnd(); ++itr) {
                allValues.insert({ itr->name.GetString(), valueToString(itr->value) });
            }
        }
        else if (m_doc.IsArray()) {
            for (auto& val : m_doc.GetArray()) {
                allValues.insert({ "", valueToString(val) });
            }
        }

        return allValues;
    }

    bool getResultAsBool(const std::string& key) const {

        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            if (val.IsBool()) {
                return val.GetBool();
            }
            else if (val.IsNumber()) {
                return val.GetDouble() != 0.0;  // zero is 'false', non-zero is 'true'
            }
            else if (val.IsString()) {
                std::string lower_val = val.GetString();
                std::transform(lower_val.begin(), lower_val.end(), lower_val.begin(), ::tolower);
                if (lower_val == "true" || lower_val == "1") return true;
                if (lower_val == "false" || lower_val == "0") return false;
                throw std::runtime_error("Cannot convert string to bool.");
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return false;
    }

    int getResultAsInt(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            if (val.IsInt()) {
                return val.GetInt();
            }
            else if (val.IsNumber()) {
                return static_cast<int>(val.GetDouble()); // Casting double type to int
            }
            else if (val.IsString()) {
                try {
                    return std::stoi(val.GetString());  // Converting string to int
                }
                catch (...) {
                    throw std::runtime_error("Cannot convert string to integer.");
                }
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return 0;
    }


    int64_t getResultAsInt64(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            if (val.IsInt64()) {
                return val.GetInt64();
            }
            else if (val.IsNumber()) {
                return static_cast<int64_t>(val.GetDouble());
            }
            else if (val.IsString()) {
                try {
                    return std::stoll(val.GetString());
                }
                catch (...) {
                    throw std::runtime_error("Cannot convert string to int64_t.");
                }
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return 0;
    }

    uint64_t getResultAsUint64(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            if (val.IsUint64()) {
                return val.GetUint64();
            }
            else if (val.IsNumber()) {
                return static_cast<uint64_t>(val.GetDouble());
            }
            else if (val.IsString()) {
                try {
                    return std::stoull(val.GetString());
                }
                catch (...) {
                    throw std::runtime_error("Cannot convert string to uint64_t.");
                }
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return 0;
    }

    double getResultAsDouble(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            if (val.IsDouble()) {
                return val.GetDouble();
            }
            else if (val.IsNumber()) {
                return static_cast<double>(val.GetInt64());  // no loss of precision
            }
            else if (val.IsString()) {
                try {
                    return std::stod(val.GetString());
                }
                catch (...) {
                    throw std::runtime_error("Cannot convert string to double.");
                }
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return 0;
    }

    std::string getResultAsString(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            if (val.IsString()) {
                return val.GetString();
            }
            else if (val.IsNumber()) {
                if (val.IsDouble()) {
                    return std::to_string(val.GetDouble());  // convert double to string
                }
                else if (val.IsInt()) {
                    return std::to_string(val.GetInt());  // convert int to string
                }
                else if (val.IsUint()) {
                    return std::to_string(val.GetUint());  // convert unsigned int to string
                }
                else if (val.IsInt64()) {
                    return std::to_string(val.GetInt64());  // convert int64_t to string
                }
                else if (val.IsUint64()) {
                    return std::to_string(val.GetUint64());  // convert uint64_t to string
                }
            }
            else if (val.IsBool()) {
                return val.GetBool() ? "true" : "false";  // convert bool to string
            }
            else if (val.IsNull()) {
                return "null";  // convert null to string
            }
            else if (val.IsObject()) {
                // This case is more complex as it involves a JSON object.
                // You may want to return the object as a serialized string,
                // or handle it differently depending on your use case.
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                val.Accept(writer);
                return std::string(buffer.GetString());
            }
            else if (val.IsArray()) {
                // This case is more complex as it involves a JSON array.
                // You may want to return the array as a serialized string,
                // or handle it differently depending on your use case.
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                val.Accept(writer);
                return std::string(buffer.GetString());
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return "";
    }
    std::string getSerializedString(const rapidjson::Value& val) const {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        val.Accept(writer);
        return std::string(buffer.GetString());
    }

    std::vector<uint8_t> getResultAsUint8Vector(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            std::vector<uint8_t> result;

            if (val.IsArray()) {
                for (auto& v : val.GetArray()) {
                    if (v.IsBool()) {
                        result.push_back(v.GetBool() ? 1 : 0);
                    }
                    else if (v.IsNull()) {
                        result.push_back(0);
                    }
                    else if (v.IsNumber()) {
                        if (v.IsDouble()) {
                            result.push_back(static_cast<uint8_t>(v.GetDouble()));
                        }
                        else if (v.IsInt()) {
                            result.push_back(static_cast<uint8_t>(v.GetInt()));
                        }
                        else if (v.IsUint()) {
                            result.push_back(static_cast<uint8_t>(v.GetUint()));
                        }
                        else if (v.IsInt64()) {
                            result.push_back(static_cast<uint8_t>(v.GetInt64()));
                        }
                        else if (v.IsUint64()) {
                            result.push_back(static_cast<uint8_t>(v.GetUint64()));
                        }
                    }
                    else if (v.IsString()) {
                        std::string s = v.GetString();
                        result.insert(result.end(), s.begin(), s.end());
                    }
                    else if (v.IsObject() || v.IsArray()) {
                        std::string serialized_string = getSerializedString(v);
                        result.insert(result.end(), serialized_string.begin(), serialized_string.end());
                    }
                    else {
                        throw std::runtime_error("Array element type mismatch.");
                    }
                }
            }
            else if (val.IsNull()) {
                result.push_back(0);
            }
            else if (val.IsBool()) {
                result.push_back(val.GetBool() ? 1 : 0);
            }
            else if (val.IsNumber()) {
                if (val.IsDouble()) {
                    result.push_back(static_cast<uint8_t>(val.GetDouble()));
                }
                else if (val.IsInt()) {
                    result.push_back(static_cast<uint8_t>(val.GetInt()));
                }
                else if (val.IsUint()) {
                    result.push_back(static_cast<uint8_t>(val.GetUint()));
                }
                else if (val.IsInt64()) {
                    result.push_back(static_cast<uint8_t>(val.GetInt64()));
                }
                else if (val.IsUint64()) {
                    result.push_back(static_cast<uint8_t>(val.GetUint64()));
                }
            }
            else if (val.IsString()) {
                std::string s = val.GetString();
                result.insert(result.end(), s.begin(), s.end());
            }
            else if (val.IsObject() || val.IsArray()) {
                std::string serialized_string = getSerializedString(val);
                result.insert(result.end(), serialized_string.begin(), serialized_string.end());
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }

            return result;
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return std::vector<uint8_t>();
    }


   


    std::vector<std::string> getResultAsStringVector(const std::string& key) const {
        if (m_doc.HasMember(key.c_str())) {
            const auto& val = m_doc[key.c_str()];
            std::vector<std::string> result;

            // If the value is an array
            if (val.IsArray()) {
                for (auto& v : val.GetArray()) {
                    if (v.IsString()) {
                        result.push_back(v.GetString());
                    }
                    else if (v.IsBool()) {
                        result.push_back(v.GetBool() ? "true" : "false");
                    }
                    else if (v.IsNumber()) {
                        result.push_back(std::to_string(v.GetDouble()));
                    }
                    else {
                        throw std::runtime_error("Array element type mismatch.");
                    }
                }
            }
            // If the value is a single object
            else if (val.IsString()) {
                result.push_back(val.GetString());
            }
            else if (val.IsBool()) {
                result.push_back(val.GetBool() ? "true" : "false");
            }
            else if (val.IsNumber()) {
                result.push_back(std::to_string(val.GetDouble()));
            }
            else {
                throw std::runtime_error("Type mismatch.");
            }

            return result;
        }
        else {
            throw std::runtime_error("Key not found.");
        }
        return std::vector<std::string>();
    }

};

