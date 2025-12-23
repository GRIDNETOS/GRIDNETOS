#pragma once
// ICodeWordProvider.h
#include <vector>
#include <string>
namespace SE {
    class CScriptEngine; // Forward declaration
};

struct CodeWord {
    std::string name;
    void (SE::CScriptEngine::* code)();
    // Add other necessary fields
};

class ICodeWordProvider {
public:
    virtual std::vector<CodeWord> getAdditionalCodeWords() = 0;
};
