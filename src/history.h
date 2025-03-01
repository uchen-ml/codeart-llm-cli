#ifndef CODEART_LLMCLI_HISTORY_H_
#define CODEART_LLMCLI_HISTORY_H_

#include <chrono>
#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"  // IWYU pragma: keep - it is in fact used

namespace codeart::llmcli {

class History {
 public:
  class Message {
   public:
    Message(std::string_view author,
            std::chrono::system_clock::time_point timestamp,
            std::string_view content_type, std::string_view content)
        : author_(author),
          timestamp_(timestamp),
          content_type_(std::move(content_type)),
          content_(std::move(content)) {}

    // Getters
    const std::string& author() const { return author_; }
    std::chrono::system_clock::time_point timestamp() const {
      return timestamp_;
    }
    const std::string& content_type() const { return content_type_; }
    const std::string& content() const { return content_; }

    // Convert message to JSON string
    nlohmann::json json() const;

    // Equality operator
    bool operator==(const Message& other) const = default;

    // Friend function for logging
    template <typename S>
    friend void AbslStringify(S& sync, const Message& message) {
      sync.Append(message.json().dump(4));
    }

   private:
    std::string author_;
    std::chrono::system_clock::time_point timestamp_;
    std::string content_type_;
    std::string content_;
  };

  static std::string TimestampToString(
      std::chrono::system_clock::time_point tp);
  static std::chrono::system_clock::time_point StringToTimestamp(
      const std::string& str);

  void AddEntry(const Message& message);
  std::span<const Message> GetEntries() const;

  friend std::ostream& operator<<(std::ostream& os,
                                  const History& history) noexcept;
  friend std::istream& operator>>(std::istream& is, History& history) noexcept;

 private:
  std::vector<Message> entries_;
};

std::ostream& operator<<(std::ostream& os, const History::Message& message);

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_HISTORY_H_
