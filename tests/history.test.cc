#include "src/history.h"

#include <chrono>
#include <sstream>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"  // IWYU pragma: keep

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace codeart::llmcli {

class HistoryTest : public ::testing::Test {};

constexpr std::string_view kExample =
    "[\n    {\n        \"author\": \"user\",\n        \"content\": "
    "\"Hello\",\n        \"content_type\": \"text\",\n        \"timestamp\": "
    "\"2025-02-28T12:35:00Z\"\n    },\n    {\n        \"author\": "
    "\"assistant\",\n        \"content\": \"Hi\",\n        \"content_type\": "
    "\"text\",\n        \"timestamp\": \"2025-02-28T12:35:00Z\"\n    }\n]";

TEST_F(HistoryTest, Serialize) {
  std::tm time{0, 35, 12, 28, 1, 125};
  std::chrono::system_clock::time_point tp =
      std::chrono::system_clock::from_time_t(std::mktime(&time));

  History history;
  history.AddEntry({"user", tp, "text", "Hello"});
  history.AddEntry({"assistant", tp, "text", "Hi"});

  std::stringstream ss;
  ss << history;
  EXPECT_EQ(ss.str(), kExample);
}

TEST_F(HistoryTest, Deserialize) {
  std::tm time{0, 35, 12, 28, 1, 125};
  std::chrono::system_clock::time_point tp =
      std::chrono::system_clock::from_time_t(std::mktime(&time));
  std::stringstream ss;
  ss << kExample;
  History deserialized;
  ss >> deserialized;
  History::Message msg = {"user", tp, "text", "Hello"};
  EXPECT_THAT(
      deserialized.GetEntries(),
      ::testing::ElementsAre(History::Message{"user", tp, "text", "Hello"},
                             History::Message{"assistant", tp, "text", "Hi"}));
}

TEST_F(HistoryTest, DeserializeInvalidJson) {
  std::stringstream ss;
  ss << "{invalid json}";

  History history;
  ss >> history;

  EXPECT_TRUE(ss.fail());
}

}  // namespace codeart::llmcli

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}