#ifndef SRC_CLIENT_H_
#define SRC_CLIENT_H_

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "absl/status/statusor.h"

#include "src/fetch.h"

namespace uchen::chat {

// Interface for LLM clients
class Model {
 public:
  virtual ~Model() = default;

  virtual std::string_view name() const = 0;

  // Queries the LLM with a prompt and multiple input contents
  virtual absl::StatusOr<std::string> Prompt(
      const Fetch& fetch, std::string_view prompt,
      absl::Span<const std::string_view> input_contents) = 0;
};

class Parameters {
 public:
  Parameters(size_t max_tokens, char* envp[]) : max_tokens_(max_tokens) {
    for (size_t i = 0; envp[i] != nullptr; ++i) {
      std::string_view env_var(envp[i]);
      auto pos = env_var.find('=');
      if (pos != std::string_view::npos) {
        env_.emplace(env_var.substr(0, pos), env_var.substr(pos + 1));
      }
    }
  }

  std::optional<std::string> GetEnv(std::string_view key) const {
    auto it = env_.find(key);
    if (it != env_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  size_t max_tokens() const { return max_tokens_; }

 private:
  size_t max_tokens_ = 1024;
  std::map<std::string, std::string, std::less<>> env_;
};

using ModelHandle = std::unique_ptr<Model>;

class ModelProvider {
 public:
  virtual ~ModelProvider() = default;
  virtual std::string_view name() const = 0;
  virtual absl::StatusOr<ModelHandle> ConnectToModel(
      std::string_view model) const = 0;
  virtual std::vector<std::string> ListModels() const = 0;
};

}  // namespace uchen::chat

#endif  // SRC_CLIENT_H_