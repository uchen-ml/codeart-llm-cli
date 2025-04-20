#ifndef SRC_MODEL_H_
#define SRC_MODEL_H_

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "absl/status/statusor.h"

#include "src/chat.h"

namespace uchen::chat {

// Interface for LLM clients
class Model {
 public:
  class Unsubscribe {
   public:
    Unsubscribe(Model* model, Chat* chat) : model_(model), chat_(chat) {}

    ~Unsubscribe() { model_->subscriptions_.erase(chat_); }

   private:
    Model* model_;
    Chat* chat_;
  };

  explicit Model(std::string name) : name_(std::move(name)) {}
  virtual ~Model() = default;

  std::string_view name() const { return name_; }

  // Connects the model to a chat session
  std::unique_ptr<Unsubscribe> Connect(std::shared_ptr<Chat> chat);

 protected:
  virtual absl::StatusOr<std::string> Send(const Message& message) = 0;

  std::string name_;
  std::unordered_map<Chat*, std::unique_ptr<Chat::Unsubscribe>> subscriptions_;
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

#endif  // SRC_MODEL_H_