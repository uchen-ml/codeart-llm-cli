#include "src/command_parser.h"

#include <sstream>

namespace codeart::llmcli {

std::optional<CommandParser::ParsedCommand> CommandParser::Parse(
    const std::string& input) {
  if (input.empty() || input[0] != '/') {
    return std::nullopt;  // Not a command, ignore for now.
  }

  std::istringstream stream(input);
  ParsedCommand parsed;
  stream >> parsed.command;  // Extract command (e.g., "/list")

  std::string arg;
  while (stream >> arg) {
    parsed.args.push_back(arg);
  }

  return parsed;
}

}  // namespace codeart::llmcli
