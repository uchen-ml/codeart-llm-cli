
#include "src/event_loop.h"
#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/synchronization/notification.h"

namespace uchen::chat {

TEST(EventLoopTest, Basic) {
  EventLoop loop;
  absl::Notification notification;
  loop.Run([&]() {
    notification.Notify();
  });
  notification.WaitForNotificationWithTimeout(absl::Milliseconds(50));
  EXPECT_TRUE(notification.HasBeenNotified());
}

TEST(EventLoopTest, TaskCanScheduleATask) {
  EventLoop loop;
  absl::Notification notification;
  loop.Run([&]() {
    loop.Run([&]() { notification.Notify(); });
  });
  notification.WaitForNotificationWithTimeout(absl::Milliseconds(50));
  EXPECT_TRUE(notification.HasBeenNotified());
}

}

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}