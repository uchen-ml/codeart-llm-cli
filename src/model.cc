#include "src/model.h"

#include "absl/log/check.h"
#include "absl/log/log.h"

namespace uchen::chat {

// Connects the model to a chat session
std::unique_ptr<Model::Unsubscribe> Model::Connect(std::shared_ptr<Chat> chat) {
  CHECK_EQ(subscriptions_.count(chat.get()), 0);
  subscriptions_.emplace(
      chat.get(), chat->Subscribe([this, chat = std::move(chat)](const Message& message) {
        if (message.provider() == this) {
          return;
        }
        auto response = Send(message);
        if (!response.ok()) {
          LOG(ERROR) << "Error sending message: " << response.status();
          return;
        }
        chat->SendMessage(Message::Origin::kAssistant, response.value(),
                          message.id(), this);
      }));
  return std::make_unique<Unsubscribe>(this, chat.get());
}

}  // namespace uchen::chat