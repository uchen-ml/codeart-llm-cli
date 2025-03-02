#ifndef CODEART_LLMCLI_CHAT_H_
#define CODEART_LLMCLI_CHAT_H_

#include <memory>
#include <unordered_map>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"

#include "src/model_manager.h"

namespace codeart::llmcli {

class Chat {
 public:
  Chat(std::shared_ptr<ModelManager> model_manager,
       absl::AnyInvocable<void(const Message&)> callback)
      : model_manager_(std::move(model_manager)),
        callback_(std::move(callback)) {}

  void DisconnectModel(const std::string& model_name) {
    connected_models_.erase(model_name);
  }

  absl::Status SendMessage(const Message& message);

 private:
  std::shared_ptr<ModelManager> model_manager_;
  absl::AnyInvocable<void(const Message&)> callback_;
  std::unordered_map<std::string, ModelInterface> connected_models_;
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_CHAT_H_