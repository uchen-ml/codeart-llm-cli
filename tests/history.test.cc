#include "src/history.h"

#include <filesystem>
#include <fstream>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/status/status.h"

#include "gtest/gtest.h"

namespace codeart::llmcli {

class HistoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Remove old test file
    std::filesystem::remove("chat_history.json");
  }

  void TearDown() override { std::filesystem::remove("chat_history.json"); }
};

TEST_F(HistoryTest, AuthorConversion) {
  EXPECT_EQ(History::AuthorToString(Author::kUser), "user");
  EXPECT_EQ(History::AuthorToString(Author::kAssistant), "assistant");
  EXPECT_EQ(History::AuthorToString(Author::kSystem), "system");

  EXPECT_TRUE(History::StringToAuthor("user").ok());
  EXPECT_TRUE(History::StringToAuthor("assistant").ok());
  EXPECT_TRUE(History::StringToAuthor("system").ok());
  EXPECT_FALSE(History::StringToAuthor("invalid").ok());

  EXPECT_EQ(History::StringToAuthor("user").value(), Author::kUser);
  EXPECT_EQ(History::StringToAuthor("assistant").value(), Author::kAssistant);
  EXPECT_EQ(History::StringToAuthor("system").value(), Author::kSystem);
}

TEST_F(HistoryTest, SaveAndLoadHistory) {
  History history;
  History::Message msg1{Author::kUser, "2025-02-28T12:34:56Z", "text",
                        "Hello, LLM"};
  History::Message msg2{Author::kAssistant, "2025-02-28T12:35:00Z", "text",
                        "Hello, User"};

  history.AddEntry(msg1);
  history.AddEntry(msg2);

  // Save history
  history.Save();

  // Load history
  auto loaded_history_or = History::Load();
  ASSERT_TRUE(loaded_history_or.ok());

  History loaded_history = loaded_history_or.value();
  auto entries = loaded_history.GetEntries();
  ASSERT_EQ(entries.size(), 2);

  EXPECT_EQ(entries[0].author, Author::kUser);
  EXPECT_EQ(entries[0].timestamp, "2025-02-28T12:34:56Z");
  EXPECT_EQ(entries[0].content, "Hello, LLM");

  EXPECT_EQ(entries[1].author, Author::kAssistant);
  EXPECT_EQ(entries[1].timestamp, "2025-02-28T12:35:00Z");
  EXPECT_EQ(entries[1].content, "Hello, User");
}

TEST_F(HistoryTest, LoadInvalidJson) {
  // Create an invalid JSON file
  std::ofstream file("chat_history.json");
  file << "{invalid json";
  file.close();

  auto history_or = History::Load();
  EXPECT_FALSE(history_or.ok());
  EXPECT_EQ(history_or.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST_F(HistoryTest, LoadEmptyFile) {
  std::ofstream file("chat_history.json");  // Create an empty file
  file.close();

  auto history_or = History::Load();
  EXPECT_TRUE(history_or.ok());
  EXPECT_EQ(history_or.value().GetEntries().size(), 0);
}

}  // namespace codeart::llmcli

int main() {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}