#pragma once
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


class CGPPArgs {
private:
    // We use a Document for our JSON object
    rapidjson::Document m_doc;

public:
    CGPPArgs() {
        m_doc.SetObject(); // Initialize as an object
    }

 

    void addArg(const std::string& name, const std::string& arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value value;
        value.SetString(arg.c_str(), m_doc.GetAllocator());
        m_doc.AddMember(key, value, m_doc.GetAllocator());
    }

    void addArg(const std::string& name, int arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value value;
        value.SetInt(arg);
        m_doc.AddMember(key, value, m_doc.GetAllocator());
    }

    void addArg(const std::string& name, double arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value value;
        value.SetDouble(arg);
        m_doc.AddMember(key, value, m_doc.GetAllocator());
    }
    void addArg(const std::string& name, uint64_t arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value value;
        value.SetUint64(arg);
        m_doc.AddMember(key, value, m_doc.GetAllocator());
    }

    void addArg(const std::string& name, bool arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value value;
        value.SetBool(arg);
        m_doc.AddMember(key, value, m_doc.GetAllocator());
    }

    void addArg(const std::string& name, const std::vector<uint8_t>& arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value array(rapidjson::kArrayType);//todo: base64 encode?

        for (const auto& item : arg) {
            rapidjson::Value value;
            value.SetInt(item);
            array.PushBack(value, m_doc.GetAllocator());
        }

        m_doc.AddMember(key, array, m_doc.GetAllocator());
    }

    void addArg(const std::string& name, const std::vector<std::string>& arg) {
        rapidjson::Value key(name.c_str(), m_doc.GetAllocator());
        rapidjson::Value array(rapidjson::kArrayType);

        for (const auto& item : arg) {
            rapidjson::Value value;
            value.SetString(item.c_str(), m_doc.GetAllocator());
            array.PushBack(value, m_doc.GetAllocator());
        }

        m_doc.AddMember(key, array, m_doc.GetAllocator());
    }



    // Function to serialize all arguments into JSON string.
    std::string serialize() const {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        m_doc.Accept(writer);
        return buffer.GetString();
    }
};
