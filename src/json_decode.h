#ifndef SRC_JSON_VALIDATOR_H_
#define SRC_JSON_VALIDATOR_H_

#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "absl/functional/any_invocable.h"
#include "absl/log/log.h"  // IWYU pragma: keep
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"

#include "nlohmann/json.hpp"  // IWYU pragma: keep

namespace uchen::json {

class DecodeError {
 public:
  explicit DecodeError(std::string_view path, std::string_view message,
                       const nlohmann::json& json)
      : message_(absl::Substitute("($0) $1 $2", path, message, json.dump())) {}

  std::string_view message() const { return message_; }

 private:
  std::string message_;
};

template <typename T>
class DecodeResult {
 public:
  DecodeResult(T&& value) : value_(std::forward<T>(value)) {}    // NOLINT
  DecodeResult(DecodeError error) : value_(std::move(error)) {}  // NOLINT

  bool ok() const { return std::holds_alternative<T>(value_); }
  const T& value() const { return std::get<T>(value_); }
  T value_or(absl::AnyInvocable<T() const> default_value) const {
    return ok() ? value() : default_value();
  }
  bool operator==(const DecodeResult& other) const = default;
  bool operator==(const T& other) const {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
      if (!ok()) {
        return std::get<DecodeError>(value_).message() == other;
      }
    }
    return ok() && value() == other;
  }

  std::string_view error() const {
    return std::get<DecodeError>(value_).message();
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const DecodeResult& result) {
    if (result.ok()) {
      sink.Append(absl::StrCat(result.value()));
    } else {
      sink.Append(absl::StrCat("Error: ", result.error()));
    }
  }

 private:
  std::variant<DecodeError, T> value_;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const DecodeResult<T>& result) {
  return os << absl::StrCat(result);
}

class JsonDecode {
 public:
  explicit JsonDecode(nlohmann::json json) : contents_(std::move(json)) {}

  JsonDecode operator[](size_t index) const;
  JsonDecode operator[](std::string_view key) const;
  DecodeResult<std::string> String() const;

  bool ok() const { return std::holds_alternative<nlohmann::json>(contents_); }

  nlohmann::json* operator->() { return &std::get<nlohmann::json>(contents_); }

 private:
  class DecodeContext {
   public:
    DecodeContext() : path_("$") {}
    DecodeContext(const DecodeContext&) = default;
    DecodeContext(DecodeContext&&) = default;
    explicit DecodeContext(std::string path) : path_(std::move(path)) {}

    DecodeContext Append(const std::string& path) const {
      return DecodeContext(path_ + path);
    }

    std::string path() const { return path_; }

   private:
    std::string path_;
  };

  JsonDecode(DecodeContext context, DecodeError error)
      : contents_(std::move(error)), context_(std::move(context)) {}
  JsonDecode(DecodeContext context, nlohmann::json json)
      : contents_(std::move(json)), context_(std::move(context)) {}

  std::variant<DecodeError, nlohmann::json> contents_;
  DecodeContext context_;
};

}  // namespace uchen::json

#endif  // SRC_JSON_VALIDATOR_H_