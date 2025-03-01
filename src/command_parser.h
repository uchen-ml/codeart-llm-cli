#ifndef CODEART_LLMCLI_COMMAND_PARSER_H_
#define CODEART_LLMCLI_COMMAND_PARSER_H_

#include <optional>
#include <string>
#include <vector>

namespace codeart::llmcli {

class CommandParser {
 public:
  struct ParsedCommand {
    std::string command;
    std::vector<std::string> args;
  };

  static std::optional<ParsedCommand> Parse(const std::string& input);
};

}  // namespace codeart::llmcli

#endif  // CODEART_LLMCLI_COMMAND_PARSER_H_
