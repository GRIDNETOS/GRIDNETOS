#ifndef DUKTAPE_WRAPPER_H
#define DUKTAPE_WRAPPER_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include "ScriptEngine.h"
#include <duktape.h>
/*
* To be added to duktape.h:
typedef duk_ret_t(*duk_custom_callback)(duk_context* ctx);
DUK_EXTERNAL_DECL void duk_set_custom_callback_before_branch(duk_context* ctx, duk_custom_callback cb);
DUK_EXTERNAL_DECL void duk_set_custom_callback_before_instruction(duk_context* ctx, duk_custom_callback cb);*/

class SE::CScriptEngine;
class CCryptoFactory;
class CGPPArgs;
class CGPPResults;
//Helper methods - BEGIN
inline static std::vector<uint8_t> duk_require_buffer_data(duk_context* ctx, duk_idx_t index) {
    duk_size_t size;
    void* buffer = duk_require_buffer_data(ctx, index, &size);
    return std::vector<uint8_t>(static_cast<uint8_t*>(buffer), static_cast<uint8_t*>(buffer) + size);
}

inline static Botan::secure_vector<uint8_t> duk_require_secure_buffer_data(duk_context* ctx, duk_idx_t index) {
    duk_size_t size;
    void* buffer = duk_require_buffer_data(ctx, index, &size);
    return Botan::secure_vector<uint8_t>(static_cast<uint8_t*>(buffer), static_cast<uint8_t*>(buffer) + size);
}


inline static  std::vector<uint8_t> duk_require_string_or_buffer_data(duk_context* ctx, duk_idx_t idx) {
    std::vector<uint8_t> data;
    if (duk_is_string(ctx, idx)) {
        const char* str = duk_require_string(ctx, idx);
        data.assign(str, str + strlen(str));
    }
    else if (duk_is_buffer_data(ctx, idx)) {
        data = duk_require_buffer_data(ctx, idx);
    }
    else {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Invalid data type. Expected a String or ArrayBuffer.");
    }
    return data;
}
//Helper methods - END
class GridScriptPPEngine : public std::enable_shared_from_this<GridScriptPPEngine> {

public:
    GridScriptPPEngine(std::shared_ptr<SE::CScriptEngine> scriptEnginePtr, std::shared_ptr<CCryptoFactory> cf,
        size_t max_heap_size = 256 * 1024, size_t max_stack_size = 8 * 1024);
    static void* custom_alloc_function(void* udata, duk_size_t size);
    static void* custom_realloc_function(void* udata, void* ptr, duk_size_t size);
    static void custom_free_function(void* udata, void* ptr);
    void init_duktape();

    static  duk_ret_t set_preserve_binding(duk_context* ctx);

    static duk_ret_t get_preserve_binding(duk_context* ctx);

    static duk_ret_t receivedGNCFloat(duk_context* ctx);

    static duk_ret_t js_getRandomNumber(duk_context* ctx);

    static duk_ret_t assets_balance(duk_context* ctx);

    static duk_ret_t assets_send(duk_context* ctx);

   
    ~GridScriptPPEngine();

    std::shared_ptr<CGPPResults> run_js(const std::string& js_code,  std::shared_ptr<CGPPArgs> = nullptr);

    std::weak_ptr<SE::CScriptEngine> getScriptEngine() const;
    void setScriptEngine(std::shared_ptr<SE::CScriptEngine> scriptEnginePtr);

    // Add getters and setters for other processScript parameters if necessary
    std::vector<uint8_t> getExecutingIdentityID()  { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_executingIdentityID; }
    void setExecutingIdentityID( std::vector<uint8_t>& id) { m_executingIdentityID = id; }

    uint64_t getTopMostValue()  { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_topMostValue; }
    void setTopMostValue(uint64_t value) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_topMostValue = value; }



    uint64_t getBlockHeight() { std::lock_guard<std::mutex> lock(mFieldsGuardian);  return m_blockHeight; }
    void setBlockHeight(uint64_t height) { std::lock_guard<std::mutex> lock(mFieldsGuardian);  m_blockHeight = height; }

    bool getResetVM() { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_resetVM; }
    void setResetVM(bool reset) { std::lock_guard<std::mutex> lock(mFieldsGuardian);  m_resetVM = reset; }

    bool getIsTransaction() { std::lock_guard<std::mutex> lock(mFieldsGuardian);  return m_isTransaction; }
    void setIsTransaction(bool transaction) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_isTransaction = transaction; }

    std::vector<uint8_t> getReceiptID()  { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_receiptID; }
    void setReceiptID(const std::vector<uint8_t>& id) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_receiptID = id; }

    bool getResetERGUsage()  { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_resetERGUsage; }
    void setResetERGUsage(bool reset) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_resetERGUsage = reset; }

    bool getGenerateBERMetadata() { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_GenerateBERMetadata; }
    void setGenerateBERMetadata(bool generate) { std::lock_guard<std::mutex> lock(mFieldsGuardian);  m_GenerateBERMetadata = generate; }

    bool getMuteDuringProcessing() { std::lock_guard<std::mutex> lock(mFieldsGuardian);  return m_muteDuringProcessing; }
    void setMuteDuringProcessing(bool mute) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_muteDuringProcessing = mute; }

    bool getIsCmdExecutingFromGUI() { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_isCmdExecutingFromGUI; }
    void setIsCmdExecutingFromGUI(bool executing) { std::lock_guard<std::mutex> lock(mFieldsGuardian);  m_isCmdExecutingFromGUI = executing; }

    bool getIsGUICommandLineTerminal()  { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_isGUICommandLineTerminal; }
    void setIsGUICommandLineTerminal(bool terminal) { std::lock_guard<std::mutex> lock(mFieldsGuardian);  m_isGUICommandLineTerminal = terminal; }

    uint64_t getMetaRequestID() { std::lock_guard<std::mutex> lock(mFieldsGuardian);  return m_metaRequestID; }
    void setMetaRequestID(uint64_t id) { m_metaRequestID = id; }

    bool getResetOnException()  { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_resetOnException; }
    void setResetOnException(bool reset) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_resetOnException = reset; }

    bool getDetachedProcessing() { std::lock_guard<std::mutex> lock(mFieldsGuardian); return m_detachedProcessing; }
    void setDetachedProcessing(bool detached) { std::lock_guard<std::mutex> lock(mFieldsGuardian); m_detachedProcessing = detached; }

    duk_ret_t custom_callback_before_branch(duk_context* ctx);
    duk_ret_t custom_callback_before_instruction(duk_context* ctx);
    static duk_ret_t GridScriptPPEngine::console_log(duk_context* ctx);
    static duk_ret_t js_confirm(duk_context* ctx);
    static duk_ret_t duk_get_callers_id(duk_context* ctx);
    static duk_ret_t console_error(duk_context* ctx);
    std::shared_ptr<CCryptoFactory> getCryptoFactory();
private:
    std::shared_ptr<CTools> mTools;
    std::shared_ptr<CCryptoFactory> mCryptoFactory;
    int32_t counter = 0;
    static void pushEmptyArrayBuffer(duk_context* ctx);
    static void pushEmptyString(duk_context* ctx);
    //GridScript 1.0 Bindings - BEGIN
    static duk_ret_t start_thread_gpp_binding(duk_context* ctx);
    static duk_ret_t get_main_thread_gpp_binding(duk_context* ctx);
    static duk_ret_t perspective_binding(duk_context* ctx);
    static duk_ret_t begin_thread_binding(duk_context* ctx);
    static duk_ret_t is_compiling_binding(duk_context* ctx);
    static duk_ret_t commit_thread_binding(duk_context* ctx);
    static duk_ret_t abort_thread_binding(duk_context* ctx);

    static duk_ret_t ready_thread_gpp_binding(duk_context* ctx);
    static duk_ret_t pause_thread_gpp_binding(duk_context* ctx);
    static duk_ret_t free_thread_gpp_binding(duk_context* ctx);
    //GridScript 1.0 Bindings - END

    //Cryptography - BEGIN
    static duk_ret_t GridScriptPPEngine::genKeyPair(duk_context* ctx);
    static duk_ret_t GridScriptPPEngine::genAddress(duk_context* ctx);
    //ECC Signatures - BEGIN
    static duk_ret_t GridScriptPPEngine::sign(duk_context* ctx);
    static duk_ret_t verifySignature(duk_context* ctx);
    //ECC Signatures - END
    static duk_ret_t getSHA2_256Vec(duk_context* ctx);
    static duk_ret_t checkAddr(duk_context* ctx);
    static duk_ret_t verifyKeys(duk_context* ctx);
    //new  - BEGIN:
    static duk_ret_t base64(duk_context* ctx);
    static duk_ret_t decBase64(duk_context* ctx);
    static duk_ret_t base58(duk_context* ctx);
    static duk_ret_t base58check(duk_context* ctx);
    static duk_ret_t decBase58check(duk_context* ctx);
    static duk_ret_t decBase58(duk_context* ctx);
    //ECIES - BEGIN
    static duk_ret_t encPub(duk_context* ctx);
    static duk_ret_t decPub(duk_context* ctx);
    //ECIES - END

    
    //Symmetric Encryption - BEGIN
    static duk_ret_t encChaCha20Boxed(duk_context* ctx);
    static duk_ret_t decChaCha20Boxed(duk_context* ctx);
    //Symmetric Encryption - END
    
    //Cryptography - END

    //System - BEGIN
    static duk_ret_t authID(duk_context* ctx);
    static duk_ret_t perspective(duk_context* ctx);
    static duk_ret_t assetsReceivedFrom(duk_context* ctx);
    static duk_ret_t codePath(duk_context* ctx);
    static duk_ret_t emptySetter(duk_context* ctx);
    static duk_ret_t user(duk_context* ctx);
    static duk_ret_t height(duk_context* ctx);
    static duk_ret_t keyHeight(duk_context* ctx);
    // User - BEGIN
    static duk_ret_t userNickname(duk_context* ctx);
    static duk_ret_t userStake(duk_context* ctx);
    static duk_ret_t userID(duk_context* ctx);
    // User - END

    //Block API - BEGIN
    static duk_ret_t paidToMiner(duk_context* ctx);
    static duk_ret_t isKeyBlockB(duk_context* ctx);
    static duk_ret_t getMinersID(duk_context* ctx);
    static duk_ret_t getErgLimit(duk_context* ctx);
    static duk_ret_t getParentID(duk_context* ctx);
    static duk_ret_t getNrOfParentsInMem(duk_context* ctx);
    static duk_ret_t getNrOfTransactions(duk_context* ctx);
    static duk_ret_t getNrOfVerifiables(duk_context* ctx);
    static duk_ret_t getNrOfReceipts(duk_context* ctx);
    static duk_ret_t getErgUsed(duk_context* ctx);
    static duk_ret_t getBlockReward(duk_context* ctx);
    static duk_ret_t getTotalBlockReward(duk_context* ctx);
    static duk_ret_t getPerspective(duk_context* ctx);
    static duk_ret_t getTotalDiffField(duk_context* ctx);
    static duk_ret_t isKeyBlock(duk_context* ctx);
    static duk_ret_t parentKeyBlockID(duk_context* ctx);
    //Block AP  - END
    
    //System - END
    
    //new  - END
    
    //File-system API - BEGIN

    static duk_ret_t Filesystem_readFile(duk_context* ctx);
    static duk_ret_t Filesystem_writeFile(duk_context* ctx);
    static duk_ret_t Filesystem_fileExists(duk_context* ctx);
    static duk_ret_t Filesystem_createDirectory(duk_context* ctx);

    //File-system API - END

    //DAO API - BEGIN
    static duk_ret_t DAO_create_poll(duk_context* ctx);
    static duk_ret_t sendGNCFloat(duk_context* ctx);
    static duk_ret_t DAO_vote_poll(duk_context* ctx);
    static duk_ret_t DAO_activate_poll(duk_context* ctx);
    //DAO API - END

    size_t heap_size;
    std::mutex mFieldsGuardian;
    static void fatal_error_handler(void* udata, const char* msg);
    static duk_ret_t js_process_script(duk_context* ctx);

    duk_context* ctx;
    std::weak_ptr<SE::CScriptEngine> scriptEnginePtr;
    size_t max_heap_size;
    size_t max_stack_size;
    std::vector<uint8_t> m_executingIdentityID;
    uint64_t m_topMostValue;
    uint64_t m_blockHeight;
    bool m_resetVM;
    bool m_isTransaction;
    std::vector<uint8_t> m_receiptID;
    bool m_resetERGUsage;
    bool m_GenerateBERMetadata;
    bool m_muteDuringProcessing;
    bool m_isCmdExecutingFromGUI;
    bool m_isGUICommandLineTerminal;
    uint64_t m_metaRequestID;
    bool m_resetOnException;
    bool m_detachedProcessing;
};
static duk_ret_t custom_callback_before_branch_trampoline(duk_context* ctx) {
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "DuktapeWrapper_ptr");
    GridScriptPPEngine* self = static_cast<GridScriptPPEngine*>(duk_get_pointer(ctx, -1));
    duk_pop_2(ctx);

    return self->custom_callback_before_branch(ctx);
}

static duk_ret_t custom_callback_before_instruction_trampoline(duk_context* ctx) {
    duk_push_global_stash(ctx);
    duk_get_prop_string(ctx, -1, "DuktapeWrapper_ptr");
    GridScriptPPEngine* self = static_cast<GridScriptPPEngine*>(duk_require_pointer(ctx, -1));
    duk_pop_2(ctx);

    return self->custom_callback_before_instruction(ctx);
}

#endif // DUKTAPE_WRAPPER_H


