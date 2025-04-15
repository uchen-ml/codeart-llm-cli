#include <string>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/strings/substitute.h"

#include "nlohmann/json.hpp"

namespace uchen::json {
namespace {

class DecodeContext {
 public:
  DecodeContext() = default;
  DecodeContext(const DecodeContext&) = default;
  DecodeContext(DecodeContext&&) = default;
  explicit DecodeContext(std::string path) : path_(std::move(path)) {}

  DecodeContext Append(const std::string& path) const {
    return DecodeContext(path_ + path);
  }

 private:
  std::string path_;
};

struct DecodeError {
  std::string message;
};

template <typename T>
class DecodeResult {
 public:
  DecodeResult(T&& value)  // NOLINT
      : value_(std::forward<T>(value)) {}
  explicit DecodeResult(DecodeError error) : value_(std::move(error)) {}

  bool ok() const {
    return std::holds_alternative<T>(value_);
  }

  const T& value() const {
    return std::get<T>(value_);
  }

 private:
  std::variant<DecodeError, T> value_;
};

class JsonDecoder {
 public:
  explicit JsonDecoder(nlohmann::json json) : contents_(std::move(json)) {}
  JsonDecoder(DecodeContext context, const char* error)
      : contents_(std::string(error)), context_(std::move(context)) {}
  JsonDecoder(DecodeContext context, nlohmann::json json)
      : contents_(std::move(json)), context_(std::move(context)) {}

  JsonDecoder ArrayElement(size_t index) {
    if (std::holds_alternative<std::string>(contents_)) {
      return *this;
    }
    auto& json = std::get<nlohmann::json>(contents_);
    if (!json.is_array()) {
      return JsonDecoder{context_, "Is not an array"};
    }
    if (index >= json.size()) {
      return JsonDecoder{
          context_,
          absl::Substitute("Trying to access index $0, but array size is $1",
                           index, json.size())};
    }
    return {context_.Append(absl::Substitute("[$0]", index)), json[index]};
  }

  DecodeResult<std::string> string() {
    if (std::holds_alternative<std::string>(contents_)) {
      return DecodeResult<std::string>{DecodeError(std::get<std::string>(contents_))};
    }
    auto& json = std::get<nlohmann::json>(contents_);
    if (!json.is_string()) {
      return DecodeResult<std::string>{DecodeError{"Is not a string"}};
    }
    return DecodeResult<std::string>{json.get<std::string>()};
  }

 private:
  std::variant<std::string, nlohmann::json> contents_;
  DecodeContext context_;
};

JsonDecoder JsonDecode(const nlohmann::json& json) { return JsonDecoder(json); }

}  // namespace

TEST(JsonValidatorTest, ArrayOfStrings) {
  // Test case for an array of strings
  std::string json = R"(["apple", "banana", "cherry"])";

  nlohmann::json parsed_json = nlohmann::json::parse(json, nullptr, false);
  ASSERT_FALSE(parsed_json.is_discarded());

  auto result = JsonDecode(parsed_json).ArrayElement(1).string();
  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.value(), "banana");
  // auto validator = JsonParse()::ArrayElement(1).String();
  // auto result = validator.Parse(json);
  // EXPECT_TRUE(result.ok());
}

}  // namespace uchen::json

int main(int argc, char** argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverity::kInfo);
  absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}