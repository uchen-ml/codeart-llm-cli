#include "src/input.h"

#include <optional>
#include <sstream>

#include <gtest/gtest.h>

namespace uchen::chat {
namespace {

TEST(InputGeneratorTest, ReadsSingleLine) {
  std::istringstream input("hello world");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "hello world");
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, ReadsMultipleLines) {
  std::istringstream input("line1\nline2\nline3");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "line1");
  EXPECT_EQ(generator(), "line2");
  EXPECT_EQ(generator(), "line3");
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, ReadsMultilineBlock) {
  std::istringstream input(
      "normal line\n^^^\nmultiline\nblock\ntext\n^^^\nfinal line");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "normal line");
  EXPECT_EQ(generator(), "multiline\nblock\ntext");
  EXPECT_EQ(generator(), "final line");
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, HandlesEmptyInput) {
  std::istringstream input("");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, HandlesNewLine) {
  std::istringstream input("\n\na");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "");
  EXPECT_EQ(generator(), "");
  EXPECT_EQ(generator(), "a");
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, HandlesUnterminatedMultilineBlock) {
  std::istringstream input("^^^\nblock without end");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "block without end");
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, HandlesMultilineBlockWithEmptyLines) {
  std::istringstream input("^^^\n\n\n^^^\n");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "\n");
  EXPECT_EQ(generator(), std::nullopt);
}

TEST(InputGeneratorTest, HandlesClosingTagAtStart) {
  Settings settings = {.multiline_open = "^^^", .multiline_close = "<<<"};
  std::istringstream input("<<<\n");
  auto generator = InputReader(input);
  EXPECT_EQ(generator(), "<<<");
  EXPECT_EQ(generator(), std::nullopt);
}

}  // namespace
}  // namespace uchen::chat