#include "src/history.h"

#include <cstddef>

namespace codeart::llmcli {

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
