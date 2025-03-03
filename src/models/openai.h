#ifndef CODEART_LLMCLI_MODELS_OPEN_AI_H_
#define CODEART_LLMCLI_MODELS_OPEN_AI_H_

#include <string>

#include "absl/status/statusor.h"

#include "src/message.h"
#include "src/model_manager.h"

namespace codeart::llmcli {

// OpenAIModelBackend implements the ModelInterface for OpenAI's API
class OpenAIModelBackend : public ModelBackend {
 public:
  // Configuration struct for OpenAI model
  struct OpenAIConfig {
    std::string api_key;
    std::string api_url = "https://api.openai.com/v1/chat/completions";
    std::string model_name = "gpt-3.5-turbo";
    float temperature = 0.7;
    int max_tokens = 1024;
  };

  // Constructor takes an OpenAIConfig object
  explicit OpenAIModelBackend(OpenAIConfig config)
      : config_(std::move(config)){};

  // No copy or move
  OpenAIModelBackend(const OpenAIModelBackend&) = delete;
  OpenAIModelBackend& operator=(const OpenAIModelBackend&) = delete;

  // Default destructor
  ~OpenAIModelBackend() override;

  // Sends a message to the OpenAI API and calls the callback with each chunk of
  // the response
  absl::StatusOr<Message> SendMessage(const Message& message) override;

  // Returns the capabilities of this model
  absl::StatusOr<ModelCapabilities> GetCapabilities() const override;

 private:
  OpenAIConfig config_;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_MODELS_OPEN_AI_H_