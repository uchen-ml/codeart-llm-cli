#ifndef SRC_CHAT_H_
#define SRC_CHAT_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <unordered_map>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"

#include "src/event_loop.h"

namespace uchen::chat {

class Message {
 public:
  enum class Origin { kAssistant, kSystem, kUser };

  Message() = default;
  Message(int id, Origin origin, std::string content,
          std::optional<int> parent_id, void* provider)
      : id_(id),
        origin_(origin),
        content_(std::move(content)),
        parent_id_(parent_id),
        provider_(provider) {}

  Message(const Message&) = default;
  Message& operator=(const Message&) = default;
  Message(Message&&) = default;
  Message& operator=(Message&&) = default;

  bool operator==(const Message& other) const = default;

  int id() const { return id_; }
  Origin origin() const { return origin_; }
  const std::string& content() const { return content_; }
  std::optional<int> parent_id() const { return parent_id_; }
  void* provider() const { return provider_; }

 private:
  int id_;
  Origin origin_;
  std::string content_;
  std::optional<int> parent_id_;
  void* provider_;
};

class Chat : public std::enable_shared_from_this<Chat> {
 public:
  using Callback = absl::AnyInvocable<void(const Message&) const>;

  class Unsubscribe {
   public:
    Unsubscribe(Chat* chat, size_t id) : chat_(chat), id_(id) {}

    ~Unsubscribe() { chat_->callbacks_.erase(id_); }

   private:
    Chat* chat_;
    size_t id_;
  };

  static std::shared_ptr<Chat> Create(std::shared_ptr<EventLoop> event_loop) {
    // Can't use std::make_shared because ctor is private.
    return std::shared_ptr<Chat>(new Chat(std::move(event_loop)));
  }

  std::optional<Message> FindMessage(int id) const;
  Message SendMessage(Message::Origin origin, std::string content,
                      std::optional<int> parent_id, void* provider);
  std::unique_ptr<Unsubscribe> Subscribe(Callback callback);

 private:
  explicit Chat(std::shared_ptr<EventLoop> event_loop)
      : event_loop_(std::move(event_loop)) {}
  mutable absl::Mutex message_mutex_;
  mutable absl::Mutex callback_mutex_;
  std::shared_ptr<EventLoop> event_loop_;
  std::atomic_int next_id_{1};
  std::unordered_map<size_t, Callback> callbacks_
      ABSL_GUARDED_BY(&callback_mutex_);
  std::unordered_map<size_t, Message> messages_
      ABSL_GUARDED_BY(&message_mutex_);
};

}  // namespace uchen::chat

#endif  // SRC_CHAT_H_