#include "src/history.h"

#include <cstddef>
#include <unordered_map>

namespace codeart::llmcli {

// Converts Author enum to string
std::string History::AuthorToString(Author author) {
  switch (author) {
    case Author::kUser:
      return "user";
    case Author::kAssistant:
      return "assistant";
    case Author::kSystem:
      return "system";
  }
  return "unknown";  // Should never happen
}

// Converts string to Author enum
absl::StatusOr<Author> History::StringToAuthor(std::string_view str) {
  static const std::unordered_map<std::string_view, Author> kAuthorMap = {
      {"user", Author::kUser},
      {"assistant", Author::kAssistant},
      {"system", Author::kSystem},
  };

  auto it = kAuthorMap.find(str);
  if (it != kAuthorMap.end()) {
    return it->second;
  }
  return absl::InvalidArgumentError("Invalid author type: " + std::string(str));
}

// Adds a new entry to history
void History::AddEntry(const Message& message) { entries_.push_back(message); }

// Retrieves all entries in history
std::span<const History::Message> History::GetEntries() const {
  return entries_;
}

// Placeholder implementations for parser
absl::StatusOr<History> HistoryParser::FromBytes(
    std::span<const std::byte> /* data */) {
  return absl::UnimplementedError("Not implemented yet");
}

std::vector<std::byte> HistoryParser::ToBytes(const History& /* history */) {
  return {};  // To be implemented
}

}  // namespace codeart::llmcli
