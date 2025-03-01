#include "history.h"

#include <fstream>

#include "absl/log/log.h"

#include "nlohmann/json.hpp"

namespace codeart::llmcli {

// static
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

// static
absl::StatusOr<Author> History::StringToAuthor(std::string_view str) {
  if (str == "user") return Author::kUser;
  if (str == "assistant") return Author::kAssistant;
  if (str == "system") return Author::kSystem;
  return absl::InvalidArgumentError("Invalid author type: " + std::string(str));
}

// static
absl::StatusOr<History> History::Load() noexcept {
  LOG(INFO) << "1";
  std::ifstream file("chat_history.json");
  if (!file) return History();

  nlohmann::json history_json;

  if (file.peek() == std::ifstream::traits_type::eof()) {
    return History();  // Empty file, return new history object.
  }

  // Use `json::parse()` with exception handling disabled
  file >> history_json;
  if (file.fail()) {
    LOG(ERROR) << "Failed to read chat history JSON.";
    return absl::InvalidArgumentError("Invalid chat history format.");
  }

  if (history_json.is_discarded()) {
    LOG(ERROR) << "Chat history JSON parsing failed.";
    return absl::InvalidArgumentError("Invalid chat history format.");
  }

  if (!history_json.is_array()) {
    LOG(ERROR) << "Chat history JSON is not an array.";
    return absl::InvalidArgumentError("Invalid chat history format.");
  }

  History history;
  for (const auto& item : history_json) {
    if (!item.is_object() || !item.contains("author") ||
        !item.contains("timestamp") || !item.contains("content_type") ||
        !item.contains("content")) {
      LOG(WARNING) << "Skipping invalid chat history entry.";
      continue;
    }

    auto author = StringToAuthor(item.value("author", ""));
    if (!author.ok()) continue;

    history.entries_.push_back({*author, item.value("timestamp", ""),
                                item.value("content_type", ""),
                                item.value("content", "")});
  }

  return history;
}

void History::AddEntry(const Message& message) {
  entries_.push_back(message);
  Save();
}

std::span<const History::Message> History::GetEntries() const {
  return entries_;
}

void History::Save() const noexcept {
  std::ofstream file("chat_history.json", std::ios::trunc);
  if (!file) {
    LOG(ERROR) << "Failed to open history file for writing.";
    return;
  }

  nlohmann::json history_json;
  for (const auto& entry : entries_) {
    history_json.push_back({{"author", AuthorToString(entry.author)},
                            {"timestamp", entry.timestamp},
                            {"content_type", entry.content_type},
                            {"content", entry.content}});
  }

  file << history_json.dump(4);
}

// static
absl::StatusOr<History> HistoryParser::FromBytes(
    std::span<const std::byte> /*data*/) {
  return absl::UnimplementedError(
      "History parsing from bytes is not implemented yet.");
}

}  // namespace codeart::llmcli
