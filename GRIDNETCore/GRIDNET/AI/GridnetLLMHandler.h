#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <windows.h>
#include <queue>

#include "llama.h"
//#include "AI/llama.cpp/common/common.h"

typedef int32_t(*llama_tokenize_ext)(const struct llama_model* model, const char* text, int32_t text_len, llama_token* tokens, int32_t n_max_tokens, bool add_bos, bool special);

class CTokenCache;
class TokenQueue {

    private:
        std::queue<std::string> queue;
        mutable std::mutex mutex;
        std::condition_variable cv;
    public:

        // Adds a new token to the queue.
        void push(const std::string& token);

        // Attempts to pop a token from the queue. Waits for a token if the queue is empty.
        std::string pop();

        // Checks if the queue is empty.
        bool empty() const;
};

class LLMHandler {

    private:
        void textGenerationWorker(const std::string& prompt, int maxLength);
        std::unique_ptr<CTokenCache> mTokenCache;
        llama_context* context; ///< Pointer to the LLM context, managing the interaction with the model.
        llama_model* model; ///< Pointer to the loaded LLM, facilitating the generation of text.
        std::atomic<bool> isGeneratingText{ false }; //This flag is used to manage the state of interactive sessions, allowing for dynamic response handling.
        std::atomic<bool> isInitialized{ false }; //This flag is used to check if LLM is initialized
        std::thread textGenerationThread; // Thread used for generating response from LLM 
        double tokensPerSecond; //This double stores how many tokens per second are generated using this model and hardware    

        // Definitions from external DLL llama.dll
        HMODULE mDLL; // HMODULE for external LLama library
        typedef int (*llama_backend_init)(bool numa);
        typedef struct llama_model_params (*llama_model_default_params)();
        typedef struct llama_context_params(*llama_context_default_params)();
        typedef struct llama_model* (*llama_load_model_from_file)(const char* path_model, llama_model_params params);
        typedef struct llama_context* (*llama_new_context_with_model)(struct llama_model* model, struct llama_context_params params);
        typedef uint32_t(*llama_n_ctx)(const struct llama_context* ctx);
        typedef struct llama_batch(*llama_batch_init)(int32_t n_tokens, int32_t embd, int32_t n_seq_max);
        typedef int32_t(*llama_decode)(struct llama_context* ctx, struct llama_batch batch);
        typedef int32_t(*llama_n_vocab)(const struct llama_model* model);
        typedef float* (*llama_get_logits_ith)(struct llama_context* ctx, int32_t i);
        typedef llama_token(*llama_sample_token_greedy)(struct llama_context* ctx, llama_token_data_array* candidates);
        typedef llama_token(*llama_token_bos)(const struct llama_model* model);
        typedef bool (*llama_should_add_bos_token)(const llama_model* model);
        typedef llama_token(*llama_token_eos)(const struct llama_model* model);
        typedef int32_t (*llama_token_to_piece_ext)(const struct llama_model* model, llama_token token, char* buf, int32_t length);
        typedef const llama_model* (*llama_get_model)(const struct llama_context* ctx);
        typedef void (*llama_batch_free)(struct llama_batch batch);
        typedef void (*llama_free)(struct llama_context* ctx);
        typedef void (*llama_free_model)(struct llama_model* model);
        typedef void (*llama_backend_free)();     

        void llamaBatchAdd(llama_batch& batch, llama_token id, llama_pos pos, const std::vector<llama_seq_id>& seq_ids, bool logits);
        void llamaBatchClear(llama_batch& batch);
        std::string llamaTokenToPiece(const llama_context* ctx, llama_token token);
        int32_t get_num_physical_cores();
        std::vector<llama_token> llamaTokenize(const llama_model* model, const std::string& text, bool add_bos, bool special);

        static void logCallback(ggml_log_level level, const char* message, void* user_data);

       

      
    public:
       
        /**
        * @brief Constructs the LLMHandler object.
        *
        * @param
        */
        LLMHandler();
        ~LLMHandler();

     

        void logHandler(ggml_log_level level, const char* message);

        /**
        * @brief Generates text based on a given prompt using the initialized LLM.
        *
        * @param prompt The initial text prompt for text generation.
        * @param maxLength The maximum length of the generated text sequence.
        * @return A string containing the generated text.
        */
        bool initializeModel(const std::string& modelPath, std::atomic_bool useCPUonly, std::int16_t gpuOffloadLayers = 1000);
        bool generateText(const std::string& prompt, int maxLength = 2048);
        //static int testLLM();
        bool isTextGenerating() const;
        std::string GetExecPath();
        TokenQueue tokenQueue; // List of generated tokens by LLM
};
