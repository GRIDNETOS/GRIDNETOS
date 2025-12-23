#pragma once
#include "pch.h"
#include "GridnetLLMHandler.h"
#include <cmath>
#include <cstdio>
#include <condition_variable>
#include <fcntl.h>
#include <io.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include "TokenCache.h"


//#include "llama.cpp/common/common.h"

// Thread-safe queue for tokens.
// Adds a new token to the queue.
void TokenQueue::push(const std::string& token) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(token);
    }
    cv.notify_one();
}

// Attempts to pop a token from the queue. Waits for a token if the queue is empty.
std::string TokenQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return !queue.empty(); });
    auto token = queue.front();
    queue.pop();
    return token;
}

// Checks if the queue is empty.
bool TokenQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex);
    return queue.empty();
}


/**
 * @brief Encapsulates the management and utilization of a Large Language Model (LLM) using the llama library.
 *
 * This class simplifies the process of loading, initializing, and querying a LLM. It gives the
 * capability to generate text based on a given prompt by leveraging the model's predictive power.
 */
/**
* @brief Constructs the LLMHandler object and initializes the LLM with the given model path.
*
* @param modelPath Path to the LLM model file.
*/
LLMHandler::LLMHandler() :mTokenCache(std::make_unique<CTokenCache>()) {
    this->context = nullptr;
    this->mDLL = nullptr;
    this->model = nullptr;
    this->tokensPerSecond = 0;
    /*if (!this->initializeModel(modelPath)) {
        throw std::runtime_error("LLMHandler initialization failed.");
    }*/
}

/**
* @brief Destroys the LLMHandler object, ensuring proper cleanup of resources.
*/
LLMHandler::~LLMHandler() {
    // Ensure the text generation thread has finished.
    if (textGenerationThread.joinable()) {
        textGenerationThread.join(); // Wait for the text generation thread to finish.
    }
    if (this->mDLL) {
        if (this->context) {
            llama_free llamaFree = (llama_free)GetProcAddress(mDLL, "llama_free");
            llamaFree(this->context);
        }
        if (this->model) {
            llama_free_model llamaFreeModel = (llama_free_model)GetProcAddress(mDLL, "llama_free_model");
            llamaFreeModel(this->model);
        }


        llama_backend_free llamaBackendFree = (llama_backend_free)GetProcAddress(mDLL, "llama_backend_free");
        llamaBackendFree();

        FreeLibrary(mDLL);
    }
}

std::string LLMHandler::GetExecPath() {
    char path[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().string();
}

void LLMHandler::llamaBatchAdd(
    struct llama_batch& batch,
    llama_token   id,
    llama_pos   pos,
    const std::vector<llama_seq_id>& seq_ids,
    bool   logits) {
    batch.token[batch.n_tokens] = id;
    batch.pos[batch.n_tokens] = pos;
    batch.n_seq_id[batch.n_tokens] = seq_ids.size();
    for (size_t i = 0; i < seq_ids.size(); ++i) {
        batch.seq_id[batch.n_tokens][i] = seq_ids[i];
    }
    batch.logits[batch.n_tokens] = logits;

    batch.n_tokens++;
}

void LLMHandler::llamaBatchClear(struct llama_batch& batch) {
    batch.n_tokens = 0;
}

std::string LLMHandler::llamaTokenToPiece(const struct llama_context* ctx, llama_token token) {

    llama_token_to_piece_ext llama_token_to_piece = (llama_token_to_piece_ext)GetProcAddress(mDLL, "llama_token_to_piece");
    llama_get_model llamaGetModel = (llama_get_model)GetProcAddress(mDLL, "llama_get_model");

    std::vector<char> result(8, 0);
    const int n_tokens = llama_token_to_piece(llamaGetModel(ctx), token, result.data(), result.size());
    if (n_tokens < 0) {
        result.resize(-n_tokens);
        int check = llama_token_to_piece(llamaGetModel(ctx), token, result.data(), result.size());
    }
    else {
        result.resize(n_tokens);
    }

    return std::string(result.data(), result.size());
}

int32_t LLMHandler::get_num_physical_cores() {
#ifdef __linux__
    // enumerate the set of thread siblings, num entries is num cores
    std::unordered_set<std::string> siblings;
    for (uint32_t cpu = 0; cpu < UINT32_MAX; ++cpu) {
        std::ifstream thread_siblings("/sys/devices/system/cpu"
            + std::to_string(cpu) + "/topology/thread_siblings");
        if (!thread_siblings.is_open()) {
            break; // no more cpus
        }
        std::string line;
        if (std::getline(thread_siblings, line)) {
            siblings.insert(line);
        }
    }
    if (!siblings.empty()) {
        return static_cast<int32_t>(siblings.size());
    }
#elif defined(__APPLE__) && defined(__MACH__)
    int32_t num_physical_cores;
    size_t len = sizeof(num_physical_cores);
    int result = sysctlbyname("hw.perflevel0.physicalcpu", &num_physical_cores, &len, NULL, 0);
    if (result == 0) {
        return num_physical_cores;
    }
    result = sysctlbyname("hw.physicalcpu", &num_physical_cores, &len, NULL, 0);
    if (result == 0) {
        return num_physical_cores;
    }
#elif defined(_WIN32)
    //TODO: Implement
#endif
    unsigned int n_threads = std::thread::hardware_concurrency();
    return n_threads > 0 ? (n_threads <= 4 ? n_threads : n_threads / 2) : 4;
}

std::vector<llama_token> LLMHandler::llamaTokenize(
    const struct llama_model* model,
    const std::string& text,
    bool   add_bos,
    bool   special) {

    llama_tokenize_ext llama_tokenize = (llama_tokenize_ext)GetProcAddress(mDLL, "llama_tokenize");

    // upper limit for the number of tokens
    int n_tokens = text.length() + add_bos;
    std::vector<llama_token> result(n_tokens);
    n_tokens = llama_tokenize(model, text.data(), text.length(), result.data(), result.size(), add_bos, special);
    if (n_tokens < 0) {
        result.resize(-n_tokens);
        int check = llama_tokenize(model, text.data(), text.length(), result.data(), result.size(), add_bos, special);
    }
    else {
        result.resize(n_tokens);
    }
    return result;
}

// Adjusted to match the full signature with log level
 void LLMHandler::logCallback(enum ggml_log_level level, const char* message, void* user_data) {
    auto handler = static_cast<LLMHandler*>(user_data);
    if (handler) {
        handler->logHandler(level, message);
    }
}

 // Actual logging handler
 void LLMHandler::logHandler(enum ggml_log_level level, const char* message) {
     // Here you could filter messages based on level or handle differently
    // std::cout << "Log Level " << level << ": " << message << std::endl;
 }

bool LLMHandler::initializeModel(const std::string& modelPath, std::atomic_bool useCPUonly, std::int16_t gpuLayersOffload) {

    if (isInitialized)
        return true;

    mDLL = LoadLibrary(TEXT("llama.dll"));

    if (!mDLL) {
        return false;
    }
    //std::stringstream buffer;
    //std::streambuf* oldCoutBuffer = std::cout.rdbuf(buffer.rdbuf());
    //freopen("llama_output.log", "w", stderr);

    // Get function addresses from external DLL
    llama_backend_init llamaBackendInit = (llama_backend_init)GetProcAddress(mDLL, "llama_backend_init");
    llama_model_default_params llamaModelFromDefaultParams = (llama_model_default_params)GetProcAddress(mDLL, "llama_model_default_params");
    llama_context_default_params llamaContextFromDefaultParams = (llama_context_default_params)GetProcAddress(mDLL, "llama_context_default_params");
    llama_load_model_from_file llamaLoadModelFromFile = (llama_load_model_from_file)GetProcAddress(mDLL, "llama_load_model_from_file");
    llama_new_context_with_model llamaNewContextWithModel = (llama_new_context_with_model)GetProcAddress(mDLL, "llama_new_context_with_model");

    // If functions are not loaded, remove lib from memory and return false
    if (!llamaBackendInit
        || !llamaModelFromDefaultParams
        || !llamaContextFromDefaultParams
        || !llamaLoadModelFromFile
        || !llamaNewContextWithModel) {
        FreeLibrary(mDLL);
        return false;
    }

    //gpt_params params;
    //params.n_gpu_layers = 1000;
    //params.interactive = true; //Interactive mode for AI
    //params.n_batch = 512;
    //params.n_predict = -1; //How many tokens should AI predict, -1 auto detect
    //params.n_keep = -1; //How many tokens should we keep in context from previous prompts, -1 for auto
  
    auto setLogCallback = (void (*)(ggml_log_callback, void*)) GetProcAddress(mDLL, "llama_log_set");
    if (setLogCallback) {
        setLogCallback(logCallback, this);
    }
    else {
        std::cerr << "Failed to set log callback function." << std::endl;
        FreeLibrary(mDLL);
        return false;
    }
    llamaBackendInit(true);
    // Define model and context parameters as per requirements.     
    llama_model_params modelParams = llamaModelFromDefaultParams();
    if (useCPUonly) {
        modelParams.n_gpu_layers = 0;
        
    }
    else {
        modelParams.n_gpu_layers = gpuLayersOffload; //How many layers should we load into GPU
    }
    
    llama_context_params contextParams = llamaContextFromDefaultParams();
    contextParams.seed = -1;
    contextParams.n_batch = 512;
    contextParams.n_threads = get_num_physical_cores();

    // Load the model from the specified file path.
    this->model = llamaLoadModelFromFile(modelPath.c_str(), modelParams);
    if (!this->model) {
        return false;
    }

    contextParams.n_ctx = 0;  //How many tokens should we handle in context, this 
    contextParams.n_threads = contextParams.n_threads; //How many CPU threads should we use for AI
    contextParams.n_threads_batch = contextParams.n_threads_batch == -1 ? contextParams.n_threads : contextParams.n_threads_batch;

    // Create a new context for the loaded model.
    this->context = llamaNewContextWithModel(this->model, contextParams);
   
    if (!this->context) {
        return false;
    }

    isInitialized = true;

    //std::cout.rdbuf(oldCoutBuffer);

    return isInitialized;
}

void LLMHandler::textGenerationWorker(const std::string& prompt, int maxLength) {

    if (!mDLL || !isInitialized) {
        return;
    }

    // Get function addresses from external DLL
    llama_tokenize_ext llama_tokenize = (llama_tokenize_ext)GetProcAddress(mDLL, "llama_tokenize");
    llama_should_add_bos_token llamaShouldAddBosToken = (llama_should_add_bos_token)GetProcAddress(mDLL, "llama_should_add_bos_token");
    llama_n_ctx llamaNCtx = (llama_n_ctx)GetProcAddress(mDLL, "llama_n_ctx");
    llama_decode llamaDecode = (llama_decode)GetProcAddress(mDLL, "llama_decode");
    llama_n_vocab llamaNVocab = (llama_n_vocab)GetProcAddress(mDLL, "llama_n_vocab");
    llama_get_logits_ith llamaGetLogitsIth = (llama_get_logits_ith)GetProcAddress(mDLL, "llama_get_logits_ith");
    llama_sample_token_greedy llamaSampleTokenGreedy = (llama_sample_token_greedy)GetProcAddress(mDLL, "llama_sample_token_greedy");
    llama_token_eos llamaTokenEos = (llama_token_eos)GetProcAddress(mDLL, "llama_token_eos");
    llama_batch_init llamaBatchInit = (llama_batch_init)GetProcAddress(mDLL, "llama_batch_init");
    llama_batch_free llamaBatchFree = (llama_batch_free)GetProcAddress(mDLL, "llama_batch_free");

    // Tokenize the input prompt to prepare for generating text.
    const bool add_bos = llamaShouldAddBosToken(this->model);
    std::vector<llama_token> embd_inp;
    bool s = mTokenCache->getBool();
    // Using TokenCache to optimize repetitive tokenization
    auto inp_pfx = mTokenCache->getTokenized(
        "system \n"
        "Welcome to GRIDNET OS, the first decentralized operating system, established by the Wizards - Guardians of the Protocol since early 2017. Unlike traditional operating systems, GRIDNET OS operates on a distributed network of nodes, ensuring no single point of failure and enhancing resistance to attacks and censorship. It leverages blockchain technology for secure, transparent, and immutable data transactions. Designed for scalability, it supports a growing number of transactions without compromising performance. Its user-centric interface makes decentralized applications accessible to a broader audience, ensuring a seamless experience. GRIDNET OS is built on principles of interoperability, sustainability, and community-driven development, redefining digital interactions with a focus on security, privacy, and user control.\n"
        "The AI you are about to converse with is integrated into the GRIDNET OS environment, embodying its principles of decentralization and security. The AI is talkative and well-versed in the specifics of GRIDNET OS. If the AI does not know the answer to a question, it will truthfully say it does not know. In addition, the AI acts like a wizard from the Guardians of the Protocol, blending the realms of advanced technology and ancient wisdom. For example, in discussing the speed of light, it might say: 'Ah, seeker of secrets, come closer and hear. In ancient times, sages discovered that the speed of light, this cosmic messenger, is 299,792,458 meters per second. This mystery unlocks the understanding of the universe and its deepest secrets. Remember these words, for they bridge the world of science and the realm of magic.' \n"
        "user \n",
        this->model, llama_tokenize, add_bos, true);

    auto promptTokens = mTokenCache->getTokenized(prompt, this->model, llama_tokenize, add_bos, false);
    auto inp_sfx = mTokenCache->getTokenized("\nassistant\n" , this->model, llama_tokenize, false, true);
    auto antipromptTokens = mTokenCache->getTokenized("[INST]" , this->model, llama_tokenize, false, true);
    auto endTextTokens = mTokenCache->getTokenized("[/INST]", this->model, llama_tokenize, false, true);

    // Combine all tokenized segments
    embd_inp.insert(embd_inp.end(), inp_pfx.begin(), inp_pfx.end());
    embd_inp.insert(embd_inp.end(), promptTokens.begin(), promptTokens.end());
    embd_inp.insert(embd_inp.end(), inp_sfx.begin(), inp_sfx.end());
    embd_inp.insert(embd_inp.end(), antipromptTokens.begin(), antipromptTokens.end());
    embd_inp.insert(embd_inp.end(), endTextTokens.begin(), endTextTokens.end());



    // Set the maximum length for the generated sequence, including the prompt.
    const int n_len = maxLength; //How many tokens can be generated as answer
    const int n_ctx = llamaNCtx(this->context);
    const int n_kv_req = embd_inp.size() + (n_len - embd_inp.size());

    // Verify if the KV cache can accommodate the entire sequence (prompt + generated tokens).
    if (embd_inp.size() + (n_len - embd_inp.size()) > n_ctx) {
        this->isGeneratingText = false;
        return;
    }

    // Initialize a batch for decoding with sufficient size.
    llama_batch batch = llamaBatchInit(n_len, 0, 1);

    // Add the tokenized prompt to the batch for initial evaluation.
    for (size_t i = 0; i < embd_inp.size(); ++i) {
        llamaBatchAdd(batch, embd_inp[i], i, { 0 }, false);
    }

    // Ensure the model generates logits for the last token of the prompt.
    batch.logits[batch.n_tokens - 1] = true;

    auto decode = llamaDecode(this->context, batch);

    if (decode != 0) {
        this->isGeneratingText = false;
        return;
    }

    int n_cur = batch.n_tokens;
    int n_decode = 0;
    const auto t_main_start = std::chrono::high_resolution_clock::now();
    std::string currToken = "";

    // Generate text until reaching the specified sequence length.
    while (n_cur <= n_len) {
        // sample the next token
        {
            auto  n_vocab = llamaNVocab(this->model);
            auto* logits = llamaGetLogitsIth(this->context, batch.n_tokens - 1);

            std::vector<llama_token_data> candidates;
            candidates.reserve(n_vocab);

            for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
                candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
            }

            llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };            

            // sample the most likely token
            const llama_token new_token_id = llamaSampleTokenGreedy(this->context, &candidates_p);
            std::string tokenString = llamaTokenToPiece(this->context, new_token_id).c_str();
            
            // is it an end of stream?
            if (new_token_id == llamaTokenEos(this->model) || n_cur == n_len) {
                break;
            }

            this->tokenQueue.push(tokenString);

            // prepare the next batch
            llamaBatchClear(batch);

            // push this new token for next evaluation
            llamaBatchAdd(batch, new_token_id, n_cur, { 0 }, true);

            n_decode += 1;
        }

        n_cur += 1;

        // evaluate the current batch with the transformer model
        if (llamaDecode(this->context, batch)) {
            return;
        }
    }

    const auto t_main_end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t_main_end - t_main_start).count();

    this->tokensPerSecond = n_decode / (duration_us / 1000000.0f);

    while (!this->tokenQueue.empty())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Free the resources allocated for the batch.
    llamaBatchFree(batch);

    // After finishing text generation, set isGeneratingText to false.
    this->isGeneratingText = false;
}

/**
* @brief Generates text based on a given prompt using the initialized LLM.
*
* @param prompt The initial text prompt for text generation.
* @param maxLength The maximum length of the generated text sequence.
* @return A string containing the generated text.
*/
bool LLMHandler::generateText(const std::string& prompt, int maxLength) {
    if (this->isGeneratingText || !this->isInitialized) {
        // Already generating text.
        return false;
    }
    this->isGeneratingText = true;

    // Ensure the previous thread has finished.
    if (this->textGenerationThread.joinable()) {
        this->textGenerationThread.join();
    }

    // Start the text generation in a separate thread.
    this->textGenerationThread = std::thread(&LLMHandler::textGenerationWorker, this, prompt, maxLength);

    return true;
}

bool LLMHandler::isTextGenerating() const
{
    return this->isGeneratingText.load();
}