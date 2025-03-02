#include "src/model_manager.h"

#include <algorithm>
#include <string>

#include "absl/log/log.h"

namespace codeart::llmcli {

// ModelInterface implementation.
absl::Status ModelInterface::SendMessage(const Message& message) {
  LOG(INFO) << "Sending message to " << model_name_ << ": " << message;
  return absl::OkStatus();
}

// ModelManager implementation.
void ModelManager::RegisterModel(std::unique_ptr<ModelBackend> model_backend) {
  std::string model_name = model_backend->Name();
  if (models_.contains(model_name)) {
    LOG(WARNING) << "Model " << model_name << " is already registered.";
    return;
  }
  models_[std::move(model_name)] = std::move(model_backend);
}

void ModelManager::UnregisterModel(std::string_view model_name) {
  if (models_.erase(std::string(model_name)) > 0) {
    LOG(INFO) << "Unregistered model: " << model_name;
  } else {
    LOG(WARNING) << "Attempted to unregister non-existent model: "
                 << model_name;
  }
}

 std::vector<std::string> ModelManager::ModelNames() const {
    std::vector<std::string> names(models_.size());
    std::transform(models_.begin(), models_.end(), names.begin(),
                   [](const auto& pair) { return pair.first; });
    return names;
}

absl::StatusOr<ModelInterface> ModelManager::ConnectModel(
    std::string_view model_name,
    std::unique_ptr<ModelDelegate> delegate) {
  auto it = models_.find(std::string(model_name));
  if (it == models_.end()) {
    return absl::NotFoundError(absl::StrCat(
                                  "Model not found: " + std::string(model_name)));
  }
  return ModelInterface(
      it->first, shared_from_this(), std::move(delegate));
}

}  // namespace codeart::llmcli
