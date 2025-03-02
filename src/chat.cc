#include "src/chat.h"

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace codeart::llmcli {
namespace {

class ChatModelDelegate : public ModelDelegate {
 public:
  ChatModelDelegate(Chat* chat, std::string_view model_name)
      : chat_(chat), model_name_(model_name) {}

  ~ChatModelDelegate() override {
    LOG(INFO) << model_name_ << " disconnected.";
    chat_->DisconnectModel(model_name_);
  }

  void OnMessage(const Message& message) override {
    LOG(INFO) << model_name_ << " < " << message;
  }

 private:
  Chat* chat_;
  std::string model_name_;
};

absl::StatusOr<std::string> GetModel(const ModelManager& model_manager) {
  auto models = model_manager.ModelNames();
  if (models.empty()) {
    return absl::FailedPreconditionError("No models configured.");
  }
  return models.front();
}

}  // namespace

absl::Status Chat::SendMessage(const Message& message) {
  auto model = GetModel(*model_manager_);
  if (!model.ok()) {
    return std::move(model).status();
  }
  auto it = connected_models_.find(model.value());
  if (connected_models_.find(model.value()) == connected_models_.end()) {
    auto connection = model_manager_->ConnectModel(
        model.value(), std::make_unique<ChatModelDelegate>(this, *model));
    if (!connection.ok()) {
      return std::move(connection).status();
    }
    it = connected_models_.emplace(*model, std::move(connection).value()).first;
  }
  return it->second.SendMessage(message);
}

}  // namespace codeart::llmcli
