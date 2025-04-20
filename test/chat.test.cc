#include "src/chat.h"

#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/synchronization/notification.h"

#include "gmock/gmock.h"
#include "src/event_loop.h"

namespace uchen::chat {
namespace {

using Origin = Message::Origin;

TEST(ChatTest, SubscribeUnsubscribe) {
  auto chat = Chat::Create(EventLoop::Create());
  using Record = std::tuple<Origin, std::string, std::optional<int>>;
  absl::Mutex mutex;
  absl::MutexLock lock(&mutex);
  std::vector<Record> messages1;
  std::vector<Record> messages2;

  auto subscribe1 = chat->Subscribe([&](const Message& message) {
    absl::MutexLock lock(&mutex);
    messages1.emplace_back(message.origin(), message.content(),
                           message.parent_id());
  });
  auto subscribe2 = chat->Subscribe([&](const Message& message) {
    absl::MutexLock lock(&mutex);
    messages2.emplace_back(message.origin(), message.content(),
                           message.parent_id());
  });
  Message msg1 = chat->SendMessage(Origin::kUser, "1", std::nullopt, nullptr);
  Message msg2 = chat->SendMessage(Origin::kSystem, "2", msg1.id(), nullptr);
  mutex.AwaitWithTimeout(
      {+[](std::vector<Record>* values) { return values->size() == 2; },
       &messages1},
      absl::Milliseconds(50));
  subscribe1.reset();
  chat->SendMessage(Origin::kSystem, "3", std::nullopt, nullptr);

  mutex.AwaitWithTimeout(
      {+[](std::vector<Record>* values) { return values->size() == 3; },
       &messages2},
      absl::Milliseconds(50));

  EXPECT_THAT(messages1, ::testing::ElementsAre(
                             std::tuple(Origin::kUser, "1", std::nullopt),
                             std::tuple(Origin::kSystem, "2", msg1.id())));
  EXPECT_THAT(messages2, ::testing::ElementsAre(
                             std::tuple(Origin::kUser, "1", std::nullopt),
                             std::tuple(Origin::kSystem, "2", msg1.id()),
                             std::tuple(Origin::kSystem, "3", std::nullopt)));
}

TEST(ChatTest, RespondToMessage) {
  auto chat = Chat::Create(EventLoop::Create());
  absl::Notification notification;
  int key = -1;
  auto subscribe = chat->Subscribe(
      [chat, &notification, key = &key](const Message& message) {
        if (message.parent_id() == std::nullopt) {
          *key = chat->SendMessage(Origin::kAssistant, message.content() + ".1",
                                   message.id(), nullptr)
                     .id();
        } else {
          notification.Notify();
        }
      });
  Message msg1 = chat->SendMessage(Origin::kUser, "1", std::nullopt, nullptr);
  notification.WaitForNotificationWithTimeout(absl::Milliseconds(20));
  EXPECT_NE(key, -1);
  EXPECT_EQ(chat->FindMessage(key)->content(), "1.1");
}

TEST(ChatTest, FindMessage) {
  auto chat = Chat::Create(EventLoop::Create());
  Message msg1 = chat->SendMessage(Origin::kSystem, "1", std::nullopt, nullptr);
  Message msg2 = chat->SendMessage(Origin::kSystem, "2", msg1.id(), nullptr);
  Message msg3 = chat->SendMessage(Origin::kUser, "3", msg2.id(), nullptr);

  EXPECT_EQ(chat->FindMessage(msg1.id()), msg1);
  EXPECT_EQ(chat->FindMessage(msg2.id()), msg2);
  EXPECT_EQ(chat->FindMessage(999), std::nullopt);
}

}  // namespace
}  // namespace uchen::chat

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}