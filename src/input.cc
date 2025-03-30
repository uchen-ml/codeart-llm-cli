#include "src/input.h"

#include <vector>

#include "absl/strings/str_join.h"

namespace uchen::chat {

std::optional<std::string> InputReader::operator()() const {
  std::string line;
  if (!std::getline(input_, line)) {
    return std::nullopt;
  }
  if (line != settings_.multiline_open) {
    return line;
  }
  std::vector<std::string> lines;
  if (line == settings_.multiline_open) {
    std::string block;
    while (std::getline(input_, line) && line != settings_.multiline_close) {
      lines.emplace_back(std::move(line));
    }
  }
  return absl::StrJoin(lines, "\n");
}

}  // namespace uchen::chat