#include "src/history.h"

#include <cstddef>
#include <cstdlib>

#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace codeart::llmcli {

// Helper to create a test message
History::Message CreateTestMessage(Author author, const std::string& timestamp,
                                   const std::string& content_type,
                                   const std::string& content) {
  return {author, timestamp, content_type, content};
}

// Test AddEntry and GetEntries
TEST(HistoryTest, AddAndRetrieveEntries) {
  History history;
  History::Message msg1 = CreateTestMessage(
      Author::kUser, "2025-02-25T12:34:56Z", "text/plain", "Hello!");
  History::Message msg2 = CreateTestMessage(
      Author::kAssistant, "2025-02-25T12:35:00Z", "text/plain", "Hi!");

  history.AddEntry(msg1);
  history.AddEntry(msg2);

  const auto& entries = history.GetEntries();
  ASSERT_EQ(entries.size(), 2);
  EXPECT_EQ(entries[0].content, "Hello!");
  EXPECT_EQ(entries[1].content, "Hi!");
}

// Test parser placeholder (should return unimplemented error)
TEST(HistoryTest, ParserUnimplemented) {
  std::vector<std::byte> dummy_data = {std::byte{0x01}, std::byte{0x02},
                                       std::byte{0x03}};
  auto result = HistoryParser::FromBytes(dummy_data);
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), absl::StatusCode::kUnimplemented);
}

TEST(HistoryTest, AuthorConversion) {
  EXPECT_EQ(History::AuthorToString(Author::kUser), "user");
  EXPECT_EQ(History::AuthorToString(Author::kAssistant), "assistant");
  EXPECT_EQ(History::AuthorToString(Author::kSystem), "system");

  EXPECT_EQ(History::StringToAuthor("user").value(), Author::kUser);
  EXPECT_EQ(History::StringToAuthor("assistant").value(), Author::kAssistant);
  EXPECT_EQ(History::StringToAuthor("system").value(), Author::kSystem);

  absl::StatusOr<Author> invalid = History::StringToAuthor("invalid");
  EXPECT_FALSE(invalid.ok());
  EXPECT_EQ(invalid.status().code(), absl::StatusCode::kInvalidArgument);
}

}  // namespace codeart::llmcli

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  return RUN_ALL_TESTS();
}