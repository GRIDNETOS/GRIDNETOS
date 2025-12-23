// CScriptEngineExt.h
#ifndef CSCRIPTENGINEEXT_H
#define CSCRIPTENGINEEXT_H

#include "ICodeWordProvider.h" // Ensure this path matches the location of ICodeWordProvider
#include <vector>
#include <mutex>
//#include "../AI/GridnetLLMHandler.h"

// Forward Declarations - BEGIN
namespace SE {
    class CScriptEngine; // Forward declaration
};
class CTools;
class LLMHandler;
// Forward Declarations - END

class CScriptEngineExt : public ICodeWordProvider {

   
public:
    bool initialize(std::shared_ptr<SE::CScriptEngine> se);
    CScriptEngineExt();
    std::vector<CodeWord> getAdditionalCodeWords() override;
    void hexi();
    static std::shared_ptr<CScriptEngineExt> getInstance();
   

private:
    std::shared_ptr<CTools> mTools;
    std::shared_ptr<CTools> getTools();
    std::mutex mFieldsGuardian;
    std::weak_ptr<SE::CScriptEngine> mScriptEngine;
    std::shared_ptr<SE::CScriptEngine> SE();
    std::shared_ptr<LLMHandler> mLLMHandler;
    static std::shared_ptr<CScriptEngineExt> sInstance;
    static std::mutex sMutex;
};

#endif // CSCRIPTENGINEEXT_H
