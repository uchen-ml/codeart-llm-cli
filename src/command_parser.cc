#include "src/command_parser.h"

#include <string>

#include "absl/strings/ascii.h"
#include "absl/strings/str_split.h"

namespace codeart::llmcli {

std::optional<CommandParser::ParsedCommand> CommandParser::Parse(
    std::string_view input) {
  std::string trimmed_input(absl::StripAsciiWhitespace(input));
  if (trimmed_input.empty() || trimmed_input[0] != '/' ||
      trimmed_input.size() == 1) {
    return std::nullopt;  // Not a command, ignore for now.
  }
  std::vector<std::string> command_args = absl::StrSplit(
      trimmed_input, absl::ByAsciiWhitespace(), absl::SkipEmpty());
  if (command_args.empty()) {
    return std::nullopt;
  }
  std::string command = std::move(command_args[0]).substr(1);
  command_args.erase(command_args.begin());
  return CommandParser::ParsedCommand{.command = std::move(command),
                                      .args = std::move(command_args)};
}

std::ostream& operator<<(std::ostream& os,
                         const CommandParser::ParsedCommand& command) {
  return os << absl::StrCat(command);
}

}  // namespace codeart::llmcli
