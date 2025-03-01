#include "src/command_parser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"  // IWYU pragma: keep

namespace codeart::llmcli {

TEST(CommandParserTest, Parse) {
  CommandParser::ParsedCommand expected = {.command = "command",
                                           .args = {"arg1", "arg2"}};
  EXPECT_THAT(CommandParser::Parse("/command arg1 arg2"),
              testing::Optional(CommandParser::ParsedCommand{
                  .command = "command", .args = {"arg1", "arg2"}}));
  EXPECT_THAT(CommandParser::Parse("  /command arg1     arg2   "),
              testing::Optional(CommandParser::ParsedCommand{
                  .command = "command", .args = {"arg1", "arg2"}}));
  EXPECT_EQ(CommandParser::Parse("a /command arg1 arg2"),
              std::nullopt);
  EXPECT_EQ(CommandParser::Parse("/"),
              std::nullopt);
  EXPECT_THAT(CommandParser::Parse("/command"),
              testing::Optional(CommandParser::ParsedCommand{
                  .command = "command", .args = {}}));
}

}  // namespace codeart::llmcli

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}