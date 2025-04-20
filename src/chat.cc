#include "src/chat.h"

#include <utility>

namespace uchen::chat {

std::optional<Message> Chat::FindMessage(int id) const {
  absl::MutexLock lock(&message_mutex_);
  auto it = messages_.find(id);
  if (it != messages_.end()) {
    return it->second;
  }
  return std::nullopt;
}

Message Chat::SendMessage(Message::Origin origin, std::string content,
                          std::optional<int> parent_id, void* provider) {
  absl::MutexLock lock(&message_mutex_);
  int id = next_id_++;
  auto result = messages_.emplace(
      id, Message(id, origin, std::move(content), parent_id, provider));
  event_loop_->Run(
      [chat = shared_from_this(), message = result.first->second]() {
        absl::MutexLock lock(&chat->callback_mutex_);
        for (const auto& [_, callback] : chat->callbacks_) {
          callback(message);
        }
      });
  return result.first->second;
}

std::unique_ptr<Chat::Unsubscribe> Chat::Subscribe(Callback callback) {
  size_t key = next_id_++;
  event_loop_->Run([chat = shared_from_this(), key,
                    callback = std::move(callback)]() mutable {
    absl::MutexLock lock(&chat->callback_mutex_);
    chat->callbacks_.emplace(key, std::move(callback));
  });
  return std::make_unique<Unsubscribe>(this, key);
}

}  // namespace uchen::chat