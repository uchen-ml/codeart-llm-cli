#include "src/json_decode.h"

#include <string>

#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"  // IWYU pragma: keep

#include "nlohmann/json.hpp"

namespace uchen::json {
namespace {}  // namespace

TEST(JsonDecoderTest, ArrayOfStrings) {
  // Test case for an array of strings
  std::string json = R"(["apple", "banana", "cherry"])";

  nlohmann::json parsed_json = nlohmann::json::parse(json, nullptr, false);
  ASSERT_FALSE(parsed_json.is_discarded());

  auto result = JsonDecode(parsed_json)[1].String();
  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.value(), "banana");
  EXPECT_FALSE(JsonDecode(parsed_json)[50].String().ok());
}

TEST(JsonDecoderTest, Field) {
  nlohmann::json json = {{"error",
                          {
                              {"code", 404},
                              {"message", "Not Found"},
                          }}};
  EXPECT_EQ(JsonDecode(json)["error"]["message"].String(), "Not Found");
  EXPECT_EQ(JsonDecode(json)["error"]["message1"].String(),
            "($.error) Key message1 not found "
            "{\"code\":404,\"message\":\"Not Found\"}");
}

}  // namespace uchen::json

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}