#ifndef CODEART_LLMCLI_HISTORY_H_
#define CODEART_LLMCLI_HISTORY_H_

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "absl/status/statusor.h"

namespace codeart::llmcli {

enum class Author { kUser, kAssistant, kSystem };

class History {
 public:
  struct Message {
    Author author;
    std::string timestamp;  // ISO 8601 format
    std::string content_type;
    std::string content;
  };

  static std::string AuthorToString(Author author);
  static absl::StatusOr<Author> StringToAuthor(std::string_view str);
  static absl::StatusOr<History> Load() noexcept;

  // Adds a new entry to history
  void AddEntry(const Message& message);

  // Retrieves all entries in history
  std::span<const Message> GetEntries() const;

  void Save() const noexcept;

 private:
  std::vector<Message> entries_;
};

// Handles conversion between History and raw byte data
class HistoryParser {
 public:
  // Converts std::span<byte> to a History instance
  static absl::StatusOr<History> FromBytes(std::span<const std::byte> data);

  // Converts a History instance to std::vector<byte>
  static std::vector<std::byte> ToBytes(const History& history);
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_HISTORY_H_
