#ifndef CODEART_LLMCLI_MODEL_INTERFACE_H_
#define CODEART_LLMCLI_MODEL_INTERFACE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

#include "src/message.h"

namespace codeart::llmcli {

class Conversation {};

struct ModelCapabilities {
  bool supports_streaming;
  size_t max_context_length;
};

class ModelBackend {
 public:
  virtual ~ModelBackend() = default;
  virtual std::string Name() const = 0;
  virtual std::unique_ptr<Conversation> StartConversation() = 0;
  virtual absl::StatusOr<Message> SendMessage(Conversation& conversation,
                                              const Message& message) = 0;
  virtual absl::StatusOr<ModelCapabilities> GetCapabilities() const = 0;
};

class ModelDelegate {
 public:
  virtual ~ModelDelegate() = default;
  virtual void OnMessage(const Message& message) = 0;
};

class ModelManager;  // Forward declaration.

class ModelInterface {
 public:
  ModelInterface(std::string model_name, std::shared_ptr<ModelManager> manager,
                 std::unique_ptr<ModelDelegate> delegate)
      : model_name_(std::move(model_name)),
        manager_(std::move(manager)),
        delegate_(std::move(delegate)) {}

  ModelInterface(const ModelInterface&) = delete;
  ModelInterface(ModelInterface&&) = default;

  absl::Status SendMessage(const Message& message);

 private:
  std::string model_name_;
  std::shared_ptr<ModelManager> manager_;
  std::unique_ptr<ModelDelegate> delegate_;
};

class ModelManager : private std::enable_shared_from_this<ModelManager> {
 public:
  static std::shared_ptr<ModelManager> Create() {
    return std::shared_ptr<ModelManager>();
  }

  void RegisterModel(std::unique_ptr<ModelBackend> model_backend);
  void UnregisterModel(std::string_view model_name);
  std::vector<std::string> ModelNames() const;
  absl::StatusOr<ModelInterface> ConnectModel(
      std::string_view model_name, std::unique_ptr<ModelDelegate> delegate);

 private:
  ModelManager() = default;

  std::unordered_map<std::string, std::unique_ptr<ModelBackend>> models_;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_MODEL_INTERFACE_H_
