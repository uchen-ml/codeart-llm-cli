#include "history.h"

#include <iomanip>
#include <sstream>

#include "absl/log/log.h"
#include "absl/strings/str_cat.h"

#include "nlohmann/json.hpp"

namespace codeart::llmcli {

std::ostream& operator<<(std::ostream& os, const History::Message& message) {
  return os << absl::StrCat(message);
}

nlohmann::json History::Message::json() const {
  return {{"author", author_},
          {"timestamp", TimestampToString(timestamp_)},
          {"content_type", content_type_},
          {"content", content_}};
}

// static
std::string History::TimestampToString(
    std::chrono::system_clock::time_point tp) {
  std::time_t time = std::chrono::system_clock::to_time_t(tp);
  std::tm tm = *std::gmtime(&time);  // Convert to UTC

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

// static
std::chrono::system_clock::time_point History::StringToTimestamp(
    const std::string& str) {
  std::tm tm = {};
  std::istringstream iss(str);
  iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

  if (iss.fail()) {
    LOG(WARNING) << "Invalid timestamp format: " << str;
    return std::chrono::system_clock::now();  // Fallback to current time
  }

#ifdef _WIN32
  std::time_t time = _mkgmtime(&tm);  // Windows equivalent of timegm()
#else
  std::time_t time = timegm(&tm);
#endif

  return std::chrono::system_clock::from_time_t(time);
}

// Serialize History to stream
std::ostream& operator<<(std::ostream& os, const History& history) noexcept {
  nlohmann::json history_json;
  for (const auto& entry : history.entries_) {
    history_json.push_back(entry.json());
  }
  os << history_json.dump(4);
  return os;
}

// Deserialize History from stream
std::istream& operator>>(std::istream& is, History& history) noexcept {
  auto history_json = nlohmann::json::parse(is, nullptr, false);
  if (is.fail() || history_json.is_discarded()) {
    LOG(ERROR) << "Failed to parse chat history JSON.";
    is.setstate(std::ios::failbit);
    return is;
  }

  if (!history_json.is_array()) {
    LOG(ERROR) << "Chat history JSON is not an array.";
    is.setstate(std::ios::failbit);
    return is;
  }

  history.entries_.clear();
  for (const auto& item : history_json) {
    if (!item.is_object() || !item.contains("author") ||
        !item.contains("timestamp") || !item.contains("content_type") ||
        !item.contains("content")) {
      LOG(WARNING) << "Skipping invalid chat history entry: " << item;
      continue;
    }
    auto author = item.value("author", "user");
    history.entries_.emplace_back(
        author, History::StringToTimestamp(item.value("timestamp", "")),
        item.value("content_type", ""), item.value("content", ""));
  }

  return is;
}

void History::AddEntry(Message message) {
  entries_.emplace_back(std::move(message));
}

}  // namespace codeart::llmcli
